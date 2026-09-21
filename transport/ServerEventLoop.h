//
// Created by zhongweiqi on 2026/1/9.
//

#ifndef ATHENA_SERVEREVENTLOOP_H
#define ATHENA_SERVEREVENTLOOP_H
#include "EventLoop.h"

namespace transport {

// boss reactor：唯一 bind/listen 业务端口，accept 后通过 uv_pipe_t(ipc=1) +
// uv_write2 把连接句柄派发给 worker EventLoop（libuv 标准的跨 loop 句柄传递，
// Windows 内部由 libuv 完成 WSADuplicateSocket，全程不触碰裸 socket 接口）。
class ServerEventLoop : public EventLoop {
public:
    ServerEventLoop(NetInterface *tcpInterFace, EventTrigger *event_trigger, int worker_num);

    ~ServerEventLoop() override;

    void bind(int port) {
        bindPort_ = port;
    }

    void run() override;

    // 第 workerIdx 个 worker 应连接的 ipc pipe 名称
    std::string pipeName(int workerIdx) const;

    // 阻塞等待全部 worker 的 ipc pipe 接入（boss loop 线程计数），超时返回 false
    bool waitWorkersReady(int timeout_ms);

    // 全部 worker 就绪后调用：在 loop 线程执行真正的 bind/listen
    void startListen();

    // 业务端口是否监听成功（bind/listen 失败或 worker 就绪超时未监听时为 false）。
    // 线程安全：startListen 的 bind/listen 都在 loop 线程执行完后才可能读到 true
    bool listeningOk() const {
        return listen_ok_.load(std::memory_order_acquire);
    }

protected:
    void onStopTriggered() override;

private:
    // loop 线程：为每个 worker 创建一个 ipc pipe server 并 listen
    bool initIpcServers();

    int indexOfPipeServer(uv_pipe_t *pipe) const;

    // loop 线程：把 accept 到的连接通过 round-robin 会话 pipe 派发给 worker
    void handoffToWorker(uv_tcp_t *client);

    static void uv_on_new_connection(uv_stream_t *server, int status);

    static void uv_on_pipe_connection(uv_stream_t *server, int status);

    static void after_handoff_cb(uv_write_t *req, int status);

    uv_tcp_t server_{};
    bool listening_ = false;
    std::atomic<bool> listen_ok_{false}; // bind+listen 成功置位，供主线程查询
    int bindPort_ = 0;
    int worker_num_ = 0;
    // uv 句柄地址不能变：构造期一次性 resize，此后不再扩容
    std::vector<uv_pipe_t> pipe_servers_;
    // worker 接入后的会话 pipe（boss loop 上，用于 uv_write2 派发），下标即 worker 序号
    std::vector<uv_pipe_t *> sessions_;
    std::string pipe_name_prefix_;
    std::atomic<int> ready_workers_{0};
    std::mutex ready_mtx_;
    std::condition_variable ready_cv_;
    size_t next_worker_ = 0;
};


} // namespace transport

#endif //ATHENA_SERVEREVENTLOOP_H
