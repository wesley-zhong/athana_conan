//
// Created by zhongweiqi on 2026/9/14.
//

#include "Actor.h"

#include <exception>
#include <chrono>

namespace core {

namespace actor {

    static std::atomic<uint64> s_actorIdSeq{1};

    Actor::Actor(std::string name, size_t mailboxCapacity)
            : _id(s_actorIdSeq.fetch_add(1)), _name(std::move(name)), _mailbox(mailboxCapacity) {
    }

    Actor::~Actor() {
        stop();
    }

    bool Actor::start() {
        bool expectFalse = false;
        if (!_isRun.compare_exchange_strong(expectFalse, true)) {
            return true; // 已在运行
        }
        try {
            _thread = std::thread(&Actor::run, this);
        } catch (const std::exception &e) {
            _isRun.store(false);
            ERR_LOG("actor [{}] start thread failed: {}", _name, e.what());
            return false;
        }
        return true;
    }

    void Actor::stop() {
        if (!_isRun.exchange(false)) {
            return;
        }
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    void Actor::run() {
        INFO_LOG("actor [{}:{}] thread start", _name, _id);
        onStart();

        uint32 idleCount = 0;
        VOID_FUN task;
        while (_isRun.load(std::memory_order_relaxed)) {
            if (_mailbox.try_dequeue(task)) {
                idleCount = 0;
                try {
                    onMessage(task);
                } catch (const std::exception &e) {
                    ERR_LOG("actor [{}:{}] onMessage exception: {}", _name, _id, e.what());
                }
                continue;
            }
            // 信箱空：先自旋让出 CPU，再逐级休眠，避免空转烧 CPU，也保证消息及时性
            ++idleCount;
            if (idleCount < 64) {
                std::this_thread::yield();
            } else if (idleCount < 1024) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        // 退出前清空信箱，已入队的消息处理完再收尾
        while (_mailbox.try_dequeue(task)) {
            try {
                onMessage(task);
            } catch (const std::exception &e) {
                ERR_LOG("actor [{}:{}] onMessage exception: {}", _name, _id, e.what());
            }
        }

        onStop();
        INFO_LOG("actor [{}:{}] thread stop", _name, _id);
    }

}

} // namespace core
