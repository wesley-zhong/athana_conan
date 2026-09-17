//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_EVENTLOOP_H
#define ATHENA_EVENTLOOP_H


#include <mutex>

#include "Channel.h"
#include "uv.h"
#include "core/common/TQueue.h"
#include "core/common/BaseType.h"
#include "IdleStateHandler.h"

namespace transport {


class NetInterface;


class EventLoop {
public:
    EventLoop(NetInterface *tcpInterFace, EventTrigger *event_trigger) : _netInterface(tcpInterFace) {
        maxPackBody = static_cast<char *>(malloc(8192));
        maxPackBodyLen = 8192;
        _eventTrigger = event_trigger;
        _loop = new uv_loop_t;
        running_ = false;
    }

    ~EventLoop()   {
    }

    void execute();

    void push(VOID_FUN func) {
        _waitTasks.push(std::move(func));
    }

    void onNewConnection(Channel *channel);

    void onClosed(Channel *channel) const;

    void onRead(Channel *channel, char *body, int len) const;

    void asyncConnect(const std::string &ip, int port);

    void startHeartbeatTimer(Channel *channel);

    // 取出一个待执行闭包，队列空返回 false
    bool pop(VOID_FUN &task) {
        return _waitTasks.tryPop(task);
    }

    virtual void run() ;

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

    // scratch buffer for one packet, grows on demand
    // returns nullptr when grow failed, caller must not use it
    char *getPacketBuff(int needLen);

    EventTrigger *event_trigger() {
        return _eventTrigger;
    }

    static void uv_alloc_cb(uv_handle_t *h, size_t s, uv_buf_t *buf);

    static void uv_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

    static void uv_on_timer(uv_timer_t *timer);

    static void uv_on_connect(uv_connect_t *req, int status);

protected:
    int initAsynEvent();

    void doRun();

    uv_loop_t *_loop;

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_;

    uv_async_t uv_async_accept; // not used in this sample (accept in main thread)
    uv_async_t uv_async_write; // used by biz threads to notify reactor for pending writes
    uv_async_t uv_async_connect;
    std::mutex write_mtx;
    core::TQueue<VOID_FUN> _waitTasks;
    std::thread t;
    NetInterface *_netInterface;
    EventTrigger *_eventTrigger;
    char *maxPackBody;
    int maxPackBodyLen;
};

} // namespace transport

#endif //ATHENA_EVENTLOOP_H
