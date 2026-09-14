//
// Created by zhongweiqi on 2026/9/14.
//

#pragma once

#include <atomic>
#include <thread>
#include <string>
#include <memory>
#include <utility>
#include <tuple>
#include <type_traits>

#include "concurrentqueue/concurrentqueue.h"
#include "../common/BaseType.h"
#include "../log/XLog.h"

namespace actor {

    // 一个 actor 一个线程，一个线程一条无锁信箱队列（moodycamel）。
    // 消息即闭包：send() 投递任意可调用体（lambda 等），由 actor 线程串行执行，业务处理无需加锁。
    // 继承并重写 onStart/onStop 参与生命周期；需要统一拦截消息（统计/上报等）时重写 onMessage()。
    class Actor {
    public:
        explicit Actor(std::string name, size_t mailboxCapacity = 8192); // mailboxCapacity 为信箱预分配容量提示

        virtual ~Actor();

        Actor(const Actor &) = delete;

        Actor &operator=(const Actor &) = delete;

        bool start();

        void stop();

        bool isRunning() const { return _isRun.load(std::memory_order_relaxed); }

        uint64 id() const { return _id; }

        const std::string &name() const { return _name; }

        // 投递闭包到信箱（线程安全、无锁、无界），由本 actor 线程串行执行。
        // 模板入参：任意 lambda/可调用体，避免调用方显式构造 std::function 的开销。
        template<typename F>
        bool execute(F &&func) {
            static_assert(std::is_constructible<VOID_FUN, F &&>::value,
                          "Actor::execute() requires a callable compatible with std::function<void()>");
            if (!isRunning()) {
                ERR_LOG("actor [{}] not running, drop task", _name);
                return false;
            }
            return _mailbox.enqueue(VOID_FUN(std::forward<F>(func)));
        }

        // 带参重载：func 的入参直接跟在后面，内部打包成 void() 闭包再入队。
        // 参数按值移动进闭包（跨线程投递安全）；需要引用语义时用 std::ref。
        template<typename F, typename... Args>
        bool execute(F &&func, Args &&... args) {
            static_assert(std::is_invocable<std::decay_t<F>, std::decay_t<Args>...>::value,
                          "Actor::execute() requires func(args...) to be invocable");
            if (!isRunning()) {
                ERR_LOG("actor [{}] not running, drop task", _name);
                return false;
            }
            return _mailbox.enqueue(VOID_FUN(
                    [f = std::forward<F>(func),
                     tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                        std::apply(f, std::move(tup));
                    }));
        }

    protected:
        // 在 actor 线程内、消息处理前调用
        virtual void onStart() {}

        // 默认直接执行闭包；重写可加统计/上报等逻辑（记得最终调用 task()）
        virtual void onMessage(VOID_FUN &task) { task(); }

        // 信箱清空后、线程退出前调用
        virtual void onStop() {}

        void run();

        uint64 _id;
        std::string _name;
        std::atomic<bool> _isRun{false};
        moodycamel::ConcurrentQueue<VOID_FUN> _mailbox;
        std::thread _thread;
    };

}
