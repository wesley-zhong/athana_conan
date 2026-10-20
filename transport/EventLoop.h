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
// 跨线程投递只有两种方式：executeOnEventLoop(task)（入队 + 唤醒）；channelPtr() 取 shared_ptr。
class EventLoop {
public:
    EventLoop(NetInterface *tcpInterFace, EventTrigger *event_trigger);

    virtual ~EventLoop();

    // 本 loop 上每条 Channel 收/发缓冲容量（loop 生命周期内固定，Channel 构造时读取）。
    // 0 表示用 Channel::DEFAULT_BUFF_SIZE(64K)。须在 start() 之前设置
    void setBuffSize(size_t recvBuffSize, size_t sendBuffSize) {
        recv_buff_size_ = recvBuffSize;
        send_buff_size_ = sendBuffSize;
    }

    void execute();

    // 任意线程：投递任务到 loop 线程执行。push + 唤醒总是成对出现，合并成一个接口防止漏唤醒
    void executeOnEventLoop(VOID_FUN func) {
        _waitTasks.push(std::move(func));
        wakeAsync(&uv_async_write);
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

    uv_loop_t *uv_loop() {
        return _loop;
    }

    // scratch buffer for one packet, grows on demand
    // returns nullptr when grow failed, caller must not use it
    char *getPacketBuff(int needLen);

    EventTrigger *event_trigger() {
        return _eventTrigger;
    }

    // ===== channel 注册表（键 = Channel::getId()）=====
    // loop 线程：登记新建 channel（EventLoop 持有 shared_ptr，直到 close 序列完成）
    void registerChannel(std::shared_ptr<Channel> channel);
    // 任意线程：取回持有中的 shared_ptr；channel 已关闭回收时返回空。
    // channel 必须仍有效（成员调用/loop 线程回调内）
    std::shared_ptr<Channel> channelPtr(Channel *channel);
    // 任意线程：按连接 id 取回 shared_ptr。id 是值类型，持有/查询都无悬垂风险，
    // 长生命周期对象（Player/PeerConn/业务缓存）跨线程引用 channel 的推荐方式
    std::shared_ptr<Channel> channelById(uint32 id);
    // 任意线程：channel 是否仍在注册表中。键已是 id，这里会解引用 channel 取 id，
    // 对已释放指针调用是 UAF——长期引用请存 id 用 channelById 判空
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

    // run() 初始化失败时调用：解除 start() 的 cv_.wait，避免启动线程永久阻塞。
    // running_ 在此被置 true，语义是“线程已到终点”，不代表 loop 可用
    void notifyStartAborted() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = true;
        }
        cv_.notify_all();
    }

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
    // 键 = Channel::getId() 而非指针：旧 channel 释放后地址可能被新 channel 复用，
    // 指针键会把 stale 查询误配到新连接；id 键只查不到（返回空）。id 在 2^32
    // 回绕前进程内唯一（见 Channel::generateId）
    std::unordered_map<uint32, std::shared_ptr<Channel> > channels_;
    core::TQueue<VOID_FUN> _waitTasks;
    std::thread t;
    NetInterface *_netInterface;
    EventTrigger *_eventTrigger;
    char *maxPackBody;
    int maxPackBodyLen;

    // setBuffSize 设置，start 前写入、之后 loop 线程只读
    size_t recv_buff_size_ = 0;
    size_t send_buff_size_ = 0;

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
