#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>

namespace core {

// RingByteBuf: SPSC (Single-Producer Single-Consumer) 场景下高效的环形字节缓冲区。
// - 容量强制/自动调整为 2 的幂次方，使用位运算（& mask）替代取模（% cap）。
// - 实际最大可用容量为 cap_ - 1（浪费 1 字节用于区分空/满）。
// - 内存序按角色取用：每个索引只由一侧线程推进（tail_=生产者、head_=消费者），
//   载入自己的索引用 relaxed，载入对方的索引用 acquire，与对方 release 存索引
//   配对，保证看到索引推进时其前面的数据读写已就绪。无法区分调用者角色的公共
//   查询（readableBytes/writableBytes）对两个索引都取 acquire，略保守但任意
//   侧调用都正确
class RingByteBuf {
public:
    explicit RingByteBuf(size_t capacity);
    ~RingByteBuf();

    // 禁止拷贝
    RingByteBuf(const RingByteBuf &) = delete;
    RingByteBuf &operator=(const RingByteBuf &) = delete;

    // 允许移动
    RingByteBuf(RingByteBuf &&other) noexcept;
    RingByteBuf &operator=(RingByteBuf &&other) noexcept;

    // 容量与大小
    // 实际可用最大字节数；moved-from 对象报 0
    size_t capacity() const noexcept { return cap_ ? cap_ - 1 : 0; }
    // 实际分配的内存大小
    size_t totalAllocatedCapacity() const noexcept { return cap_; }

    // 可读字节数。生产/消费两侧都会调用（如 Channel::eventLoopWrite 判空决定
    // 是否 kick 发送），无法假设调用者角色，两个索引都取 acquire。
    // 高频路径（每帧解析多次），故内联在头文件
    size_t readableBytes() const noexcept {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return (t - h) & mask_;
    }

    size_t writableBytes() const noexcept {
        // moved-from 对象 cap_ == 0，报告不可写，避免 (cap_ - 1) 无符号下溢成 SIZE_MAX
        if (cap_ == 0) return 0;
        // 实际可用最大空间为 cap_ - 1
        return (cap_ - 1) - readableBytes();
    }

    // 写入与读取
    bool tryWrite(const void *src, size_t len);
    size_t read(void *dst, size_t len);
    size_t peek(void *dst, size_t len) const;

    // 零拷贝接口
    uint8_t *linearReadablePtr(size_t *outLen) noexcept;
    uint8_t *linearWritablePtr(size_t *outLen) noexcept;

    // 移动读写指针。溢出返回 false 而非抛异常：本类型用于 libuv 回调路径，
    // 异常抛穿 C 栈是 UB，由调用方按协议错误处理（关连接/丢帧）
    bool advanceReadIndex(size_t n) noexcept;
    bool advanceWriteIndex(size_t n) noexcept;

    // 清空缓冲区
    void clear() noexcept;

    // 内存分配辅助
    static void *je_malloc(size_t size);
    static void je_free(void *ptr);

private:
    // 计算大于等于 n 的最小 2 的幂次方
    static constexpr size_t ceilToPowerOfTwo(size_t n) noexcept {
        if (n <= 2) return n < 1 ? 2 : n;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
        n |= n >> 32;
#endif
        return n + 1;
    }

private:
    uint8_t *buf_{nullptr};            // 内存基址
    size_t cap_{0};                    // 实际分配容量 (必须是 2 的幂)
    size_t mask_{0};                   // 掩码 (cap_ - 1)
    std::atomic<size_t> head_{0};     // 读索引（消费者推进）
    std::atomic<size_t> tail_{0};     // 写索引（生产者推进）
};

} // namespace core
