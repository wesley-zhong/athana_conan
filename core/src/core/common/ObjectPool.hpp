//
// Created by zhongweiqi on 2025/10/16.
//
#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <utility>
#include <atomic>
#include <new>
#include "../log/XLog.h"

// 对象池生命周期契约：
// - acquire：命中空闲链时先 std::destroy_at 结束旧实例，再原地构造新实例；未命中直接 new。
// - release：只把对象归还空闲链，不析构。
// - 每块存储上的实例有且仅有一次析构：下一次 acquire（destroy_at）或池销毁（unique_ptr）。
//   这样既避免了"acquire 覆盖未析构实例"的 UB，也避免了"release 已析构 + 池销毁再析构"
//   的双析构 UB。注意：参与池化的类型，其构造函数不应抛异常。
template<typename T>
class ObjectPool {
    struct Node {
        T *obj;
        Node *next;
    };

    // 说明：原先采用裸指针 CAS 的无锁 Treiber 栈，但对象池"节点弹出后又被压回同一位置"
    // 的使用模式天然存在 ABA 风险（多 EventLoop 线程并发取还时可能丢失节点，甚至把同一
    // 对象交给两个线程）。出入链本身只有几十纳秒，相对使用方（网络 IO、protobuf 解析）
    // 的开销可忽略，故改为互斥锁实现。
    std::mutex _mutex;
    Node *_head{nullptr};     // 空闲对象链（node->obj 指向可复用对象）
    Node *_nodePool{nullptr}; // 已弹出的空闲 Node 链，release 时优先复用

    std::vector<std::unique_ptr<T> > _ownedObjects;
    std::vector<std::unique_ptr<Node> > _ownedNodes;

    // stats（atomic 仅让查询接口免锁读取；所有修改都在 _mutex 内）
    std::atomic<size_t> _freeCount{0};
    std::atomic<size_t> _allocCountObj{0};
    std::atomic<size_t> _allocCountNode{0};
    std::atomic<size_t> _hitCount{0};
    std::atomic<size_t> _missCount{0};

    size_t _maxSize = 0; // 0 = unlimited；限制的是空闲链长度，miss 路径的对象总量不受限
    bool _debug = false;

public:
    class PoolObjRef;

    explicit ObjectPool(size_t initial = 0, size_t maxSize = 0)
        : _maxSize(maxSize) {
        reserve(initial);
    }

    void setDebug(bool enabled) noexcept { _debug = enabled; }

    // 初始化预分配 n 个默认构造的对象
    void reserve(size_t n) {
        for (size_t i = 0; i < n; ++i) {
            auto objUP = std::make_unique<T>();
            T *obj = objUP.get();

            auto nodeUP = std::make_unique<Node>();
            Node *node = nodeUP.get();
            node->obj = obj;

            std::lock_guard<std::mutex> lk(_mutex);
            _ownedObjects.push_back(std::move(objUP));
            _ownedNodes.push_back(std::move(nodeUP));
            node->next = _head;
            _head = node;

            _freeCount.fetch_add(1, std::memory_order_relaxed);
            _allocCountObj.fetch_add(1, std::memory_order_relaxed);
            _allocCountNode.fetch_add(1, std::memory_order_relaxed);
        }
    }

    template<typename... Args>
    PoolObjRef acquire(Args &&... args) {
        return PoolObjRef(acquireImpl(std::forward<Args>(args)...), this);
    }

    template<typename... Args>
    T *acquirePtr(Args &&... args) {
        return acquireImpl(std::forward<Args>(args)...);
    }

    template<typename... Args>
    std::unique_ptr<PoolObjRef> acquireUniquePtr(Args &&... args) {
        return std::make_unique<PoolObjRef>(acquireImpl(std::forward<Args>(args)...), this);
    }

    // 归还对象，不析构（见文件头注释）。空闲链达到 _maxSize 时丢弃——对象仍由池持有，
    // 池销毁时统一析构，不会泄漏。Node 分配失败时同样丢弃，保证 noexcept。
    void release(T *obj) noexcept {
        if (!obj) return;

        std::lock_guard<std::mutex> lk(_mutex);

        if (_debug) {
            // 重复归还会让两个使用者拿到同一实例，debug 模式下直接拒绝
            for (Node *n = _head; n; n = n->next) {
                if (n->obj == obj) {
                    ERR_LOG("[Pool] double release detected obj {}", fmt::ptr(obj));
                    return;
                }
            }
        }

        if (_maxSize > 0 && _freeCount.load(std::memory_order_relaxed) >= _maxSize) {
            INFO_LOG("[Pool] Drop obj {}  maxSize reached", fmt::ptr(obj));
            return;
        }

        Node *node = _nodePool;
        if (node) {
            _nodePool = node->next;
        } else {
            node = new (std::nothrow) Node();
            if (!node) {
                ERR_LOG("[Pool] alloc node failed, drop obj {}", fmt::ptr(obj));
                return;
            }
            _allocCountNode.fetch_add(1, std::memory_order_relaxed);
        }
        node->obj = obj;
        node->next = _head;
        _head = node;

        _freeCount.fetch_add(1, std::memory_order_relaxed);
    }

    // ===== 统计接口 =====
    size_t freeCount() const noexcept { return _freeCount.load(); }
    size_t maxSize() const noexcept { return _maxSize; }
    size_t hitCount() const noexcept { return _hitCount.load(); }
    size_t missCount() const noexcept { return _missCount.load(); }
    size_t allocCountObj() const noexcept { return _allocCountObj.load(); }
    size_t allocCountNode() const noexcept { return _allocCountNode.load(); }

    void printStats() const {
        INFO_LOG("==== ObjectPool Stats ====");
        INFO_LOG("freeCount     :   {} ", freeCount());
        INFO_LOG("hitCount      :   {} ", hitCount());
        INFO_LOG("missCount     :   {} ", missCount());
        INFO_LOG("allocCountObj :   {} ", allocCountObj());
        INFO_LOG("allocCountNode:   {} ", allocCountNode());
        INFO_LOG("==========================");
    }

private:
    template<typename... Args>
    T *acquireImpl(Args &&... args) {
        T *obj = nullptr;
        {
            std::lock_guard<std::mutex> lk(_mutex);
            if (_head) {
                Node *n = _head;
                _head = n->next;
                obj = n->obj;
                // 节点回收复用（原先 acquire 直接丢弃节点，导致 Node 无界增长）
                n->next = _nodePool;
                _nodePool = n;

                _freeCount.fetch_sub(1, std::memory_order_relaxed);
                _hitCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        if (obj) {
            std::destroy_at(obj);                    // 结束旧实例生命周期（恰好一次）
            new(obj) T(std::forward<Args>(args)...); // 原地构造新实例
            return obj;
        }

        _missCount.fetch_add(1, std::memory_order_relaxed);
        auto up = std::make_unique<T>(std::forward<Args>(args)...);
        obj = up.get();
        {
            std::lock_guard<std::mutex> lk(_mutex);
            _ownedObjects.push_back(std::move(up));
        }
        _allocCountObj.fetch_add(1, std::memory_order_relaxed);
        return obj;
    }
};

// RAII handle
template<typename T>
class ObjectPool<T>::PoolObjRef {
public:
    PoolObjRef() noexcept : _obj(nullptr), _pool(nullptr) {
    }

    PoolObjRef(T *obj, ObjectPool *pool) noexcept : _obj(obj), _pool(pool) {
    }

    PoolObjRef(const PoolObjRef &) = delete;

    PoolObjRef &operator=(const PoolObjRef &) = delete;

    PoolObjRef(PoolObjRef &&o) noexcept : _obj(o._obj), _pool(o._pool) {
        o._obj = nullptr;
        o._pool = nullptr;
    }

    PoolObjRef &operator=(PoolObjRef &&o) noexcept {
        if (this != &o) {
            release();
            _obj = o._obj;
            _pool = o._pool;
            o._obj = nullptr;
            o._pool = nullptr;
        }
        return *this;
    }

    ~PoolObjRef() { release(); }

    T *operator->() const noexcept { return _obj; }
    T &operator*() const noexcept { return *_obj; }
    T *get() const noexcept { return _obj; }
    explicit operator bool() const noexcept { return _obj != nullptr; }

    void release() noexcept {
        if (_obj && _pool) {
            _pool->release(_obj);
            _obj = nullptr;
            _pool = nullptr;
        }
    }

private:
    T *_obj;
    ObjectPool *_pool;
};


namespace ObjPool {
    template<typename T>
    ObjectPool<T> &GetPool() {
        static ObjectPool<T> s_pool(0, 4096);
        return s_pool;
    }


    template<typename T, typename... Args>
    typename ObjectPool<T>::PoolObjRef acquire(Args &&... args) {
        return GetPool<T>().acquire(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T *acquirePtr(Args &&... args) {
        return GetPool<T>().acquirePtr(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    std::unique_ptr<typename ObjectPool<T>::PoolObjRef> acquireUniquePtr(Args &&... args) {
        return GetPool<T>().acquireUniquePtr(std::forward<Args>(args)...);
    }

    template<typename T>
    void release(T *ptr) {
        GetPool<T>().release(ptr);
    }

    template<typename T>
    class PoolObjClass {
    public:
        template<typename... Args>
        static T *create(Args &&... args) {
            return acquirePtr<T>(std::forward<Args>(args)...);
        }

        template<typename... Args>
        static typename ObjectPool<T>::PoolObjRef claim(Args &&... args) {
            return acquire<T>(std::forward<Args>(args)...);
        }

        static void recycle(T *ptr) {
            release<T>(ptr);
        }
    };
}
