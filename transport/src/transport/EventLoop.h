//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_EVENTLOOP_H
#define ATHENA_EVENTLOOP_H


#include <mutex>

#include "Channel.h"
#include "uv.h"
#include "core/common/ThreadPool.h"
#include "IdleStateHandler.h"


class NetInterface;


class EventLoop {
public:
    EventLoop(NetInterface *tcpInterFace, EventTrigger *event_trigger) : _netInterface(tcpInterFace) {
        maxPackBody = static_cast<char *>(malloc(8192));
        _eventTrigger = event_trigger;
    }

    ~EventLoop() {
    }

    void asyncAccept(uv_os_sock_t client);

    void execute();

    void push(std::function<void()> func) {
        auto it = Thread::RunTask::create(func);
        _waitTasks.push(it);
    }

    void onNewConnection(Channel *channel);

    void onClosed(Channel *channel) const;

    void onRead(Channel *channel, char *body, int len) const;

    void asyncConnect(const std::string &ip, int port);

    void startHeartbeatTimer(Channel *channel);

    Thread::TaskPtr pop() {
        Thread::TaskPtr run_task;
        bool ret = _waitTasks.tryPop(run_task);
        if (ret) {
            return run_task;
        }
        return nullptr;
    }

    void run();

    void start();

    void async_write_task() {
        uv_async_send(&uv_async_write);
    }

    void async_accept_task() {
        uv_async_send(&uv_async_accept);
    }

    void async_connect_task() {
        uv_async_send(&uv_async_connect);
    }

    uv_loop_t *uv_loop() {
        return _loop;
    }

    char *getPacketBuff() {
        return maxPackBody;
    }
    EventTrigger *event_trigger() {
        return _eventTrigger;
    }

    static void uv_alloc_cb(uv_handle_t *h, size_t s, uv_buf_t *buf);

    static void uv_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

    static void uv_on_timer(uv_timer_t *timer);

    static void uv_on_connect(uv_connect_t *req, int status);

private:
    uv_loop_t *_loop;
    uv_async_t uv_async_accept; // not used in this sample (accept in main thread)
    uv_async_t uv_async_write; // used by biz threads to notify reactor for pending writes
    uv_async_t uv_async_connect;
    std::mutex write_mtx;
    TQueue<Thread::TaskPtr> _waitTasks;
    std::thread t;
    NetInterface *_netInterface;
    EventTrigger *_eventTrigger;
    char *maxPackBody;
};

#endif //ATHENA_EVENTLOOP_H
