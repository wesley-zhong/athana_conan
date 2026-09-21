//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_EVENTLOOP_H
#define ATHENA_EVENTLOOP_H


#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Channel.h"
#include "uv.h"
#include "common/TQueue.h"
#include "common/BaseType.h"
#include "IdleStateHandler.h"

namespace transport {

class NetInterface;

// stop 异步回调触发 onStopTriggered（protected，声明为友元）
void async_stop_cb(uv_async_t *handler);


// 单个 reactor。除 async_send 相关接口外，所有句柄操作都发生在本 loop 线程。
// 跨线程投递只有两种方式：push(task) + async_write_task() 唤醒；channelPtr() 取 shared_ptr。
class EventLoop {
public:
    EventLoop(NetInterface *tcpInterFace, EventTrigger *event_trigger);

    virtual ~EventLoop();

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

    virtual void run();

    void start();

    // 线程安全：请求优雅停机。loop 线程会关闭所有 channel 与句柄后自然退出 uv_run。
    // 必须在 start() 之后调用。
    void stop();

    // 等待 loop 线程退出（与 start() 配对）
    void join();

    void async_write_task() {
        wakeAsync(&uv_async_write);
    }

    void async_accept_task() {
        wakeAsync(&uv_async_accept);
    }

    void async_connect_task() {
        wakeAsync(&uv_async_connect);
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

    // ===== channel 注册表 =====
    // loop 线程：登记新建 channel（EventLoop 持有 shared_ptr，直到 close 序列完成）
    void registerChannel(std::shared_ptr<Channel> channel);
    // 任意线程：取回持有中的 shared_ptr；channel 已关闭回收时返回空
    std::shared_ptr<Channel> channelPtr(Channel *channel);
    // 任意线程：channel 是否仍在注册表中（对已释放的裸指针调用仍有 UAF 风险，
    // 优先使用 channelPtr 拿 shared_ptr）
    bool isLive(Channel *channel);

    // ===== worker 模式（boss/worker 多 reactor）=====
    // 向 boss loop 的 ipc pipe 发起连接（启动前调用，任务在 run 后执行）
    void connectIpc(const std::string &pipe_name);
    // loop 线程：把一个已 accept 的 uv_tcp_t 纳入本 loop 管理并通知业务层
    void adoptConnection(uv_tcp_t *client);

    static void uv_alloc_cb(uv_handle_t *h, size_t s, uv_buf_t *buf);

    static void uv_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

    static void uv_on_timer(uv_timer_t *timer);

    static void uv_on_connect(uv_connect_t *req, int status);

protected:
    int initAsynEvent();

    void doRun();

    // loop 线程：stop 触发后关闭本 loop 的句柄；派生类先关闭自己的句柄再调基类
    virtual void onStopTriggered();

    uv_loop_t *_loop;

private:
    friend void async_stop_cb(uv_async_t *handler);

    void wakeAsync(uv_async_t *async) {
        // 停机后 asyncs 会被 uv_close，loop 未启动时 asyncs 尚未初始化，
        // 这两种情况下 uv_async_send 都是未定义行为，必须拦下
        if (!stopping_.load(std::memory_order_acquire) &&
            asyncs_inited_.load(std::memory_order_acquire)) {
            uv_async_send(async);
        }
    }

    // loop 线程：tcp/timer 的 close 回调都完成后解除注册表持有
    friend class Channel;

    void releaseChannel(Channel *channel);

    static void uv_ipc_connect_cb(uv_connect_t *req, int status);

    static void uv_ipc_read_cb(uv_stream_t *pipe, ssize_t nread, const uv_buf_t *buf);

    static void ipc_alloc_cb(uv_handle_t *h, size_t s, uv_buf_t *buf);

    // loop 线程：关闭 worker 端 ipc pipe（停机/对端断开时）。ipc pipe 是 active
    // handle，不关则 uv_run 永不返回，AthenaTcpServer::stop 的 join 会挂死
    void closeIpcPipe();

    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_;

    std::atomic<bool> stopping_{false};
    std::atomic<bool> asyncs_inited_{false};

    uv_async_t uv_async_accept; // accept/connect 任务队列唤醒
    uv_async_t uv_async_write; // used by biz threads to notify reactor for pending writes
    uv_async_t uv_async_connect;
    uv_async_t uv_async_stop;

    std::mutex live_mtx_;
    std::unordered_map<Channel *, std::shared_ptr<Channel> > channels_;
    core::TQueue<VOID_FUN> _waitTasks;
    std::thread t;
    NetInterface *_netInterface;
    EventTrigger *_eventTrigger;
    char *maxPackBody;
    int maxPackBodyLen;

    // ipc worker 端：连接 boss 用于接收传递过来的连接句柄
    uv_pipe_t ipc_pipe_{};
    uv_connect_t ipc_connect_req_{};
    char ipc_scratch_[256];
    std::string ipc_pipe_name_;
    bool ipc_inited_ = false;    // loop 线程：uv_pipe_init 成功后置位
    bool ipc_closing_ = false;   // loop 线程：closeIpcPipe 已发起，防重复 close
};


} // namespace transport

#endif //ATHENA_EVENTLOOP_H
