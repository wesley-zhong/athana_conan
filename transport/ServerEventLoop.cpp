//
// Created by zhongweiqi on 2026/1/9.
//

#include "ServerEventLoop.h"
#include "log/XLog.h"
#include <chrono>
#include <cstdio>

namespace transport {

ServerEventLoop::ServerEventLoop(NetInterface *tcpInterFace, EventTrigger *event_trigger, int worker_num)
    : EventLoop(tcpInterFace, event_trigger) {
    if (worker_num < 1) {
        worker_num = 1;
    }
    worker_num_ = worker_num;
    pipe_servers_.resize(worker_num_);
    char buf[64];
    snprintf(buf, sizeof(buf), "athena-%d", (int) uv_os_getpid());
    pipe_name_prefix_ = buf;
}

ServerEventLoop::~ServerEventLoop() = default;

std::string ServerEventLoop::pipeName(int workerIdx) const {
    char buf[160];
#ifdef _WIN32
    snprintf(buf, sizeof(buf), "\\\\.\\pipe\\%s-%d", pipe_name_prefix_.c_str(), workerIdx);
#else
    snprintf(buf, sizeof(buf), "/tmp/%s-%d.sock", pipe_name_prefix_.c_str(), workerIdx);
#endif
    return buf;
}

bool ServerEventLoop::waitWorkersReady(int timeout_ms) {
    std::unique_lock<std::mutex> lk(ready_mtx_);
    return ready_cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms), [this] {
        return ready_workers_.load() >= worker_num_;
    });
}

void ServerEventLoop::run() {
    initAsynEvent();
    if (!initIpcServers()) {
        ERR_LOG("boss init ipc servers failed, worker dispatch unavailable");
    }
    doRun();
}

int ServerEventLoop::indexOfPipeServer(uv_pipe_t *pipe) const {
    for (int i = 0; i < worker_num_; i++) {
        if (&pipe_servers_[i] == pipe) {
            return i;
        }
    }
    return -1;
}

bool ServerEventLoop::initIpcServers() {
    sessions_.assign(worker_num_, nullptr);
    for (int i = 0; i < worker_num_; i++) {
        uv_pipe_t &ps = pipe_servers_[i];
        // 注意：Windows 上 libuv 禁止 ipc pipe 作为监听端(uv__pipe_listen 返回 EINVAL)，
        // 监听端必须 ipc=0；accept 出的会话 pipe 与 worker 端才是 ipc=1（传句柄的两端都要求 ipc）
        int ret = uv_pipe_init(_loop, &ps, 0);
        if (ret != 0) {
            ERR_LOG("uv_pipe_init failed: {}", uv_err_name(ret));
            return false;
        }
        ps.data = this;
        std::string name = pipeName(i);
#ifndef _WIN32
        // 清理上次异常退出残留的 socket 文件，否则 bind 会失败
        uv_fs_t fs_req;
        uv_fs_unlink(_loop, &fs_req, name.c_str(), nullptr);
        uv_fs_req_cleanup(&fs_req);
#endif
        ret = uv_pipe_bind(&ps, name.c_str());
        if (ret != 0) {
            ERR_LOG("uv_pipe_bind {} failed: {}", name, uv_err_name(ret));
            return false;
        }
        ret = uv_listen((uv_stream_t *) &ps, 1, uv_on_pipe_connection);
        if (ret != 0) {
            ERR_LOG("uv_listen ipc {} failed: {}", name, uv_err_name(ret));
            return false;
        }
    }
    return true;
}

void ServerEventLoop::startListen() {
    // 等全部 worker 就绪后再监听业务端口，保证 accept 到的连接一定能派发出去
    executeOnEventLoop([this]() {
        int ret = uv_tcp_init(_loop, &server_);
        if (ret != 0) {
            ERR_LOG("uv_tcp_init failed: {}", uv_err_name(ret));
            return;
        }
        server_.data = this;
        sockaddr_in addr;
        ret = uv_ip4_addr("0.0.0.0", bindPort_, &addr);
        if (ret != 0) {
            ERR_LOG("uv_ip4_addr failed: {}", uv_err_name(ret));
            return;
        }
        ret = uv_tcp_bind(&server_, reinterpret_cast<const sockaddr *>(&addr), 0);
        if (ret != 0) {
            ERR_LOG("#### server bind port ={} failed: {}", bindPort_, uv_err_name(ret));
            return;
        }
        ret = uv_listen((uv_stream_t *) &server_, 1024, uv_on_new_connection);
        if (ret != 0) {
            ERR_LOG("#### server listen port ={} failed: {}", bindPort_, uv_err_name(ret));
            return;
        }
        listening_ = true;
        listen_ok_.store(true, std::memory_order_release);
        INFO_LOG("#### server listening port ={} (boss + {} workers)", bindPort_, worker_num_);
    });
}

// boss：accept 到新连接后派发给 worker
void ServerEventLoop::uv_on_new_connection(uv_stream_t *server, int status) {
    auto *self = static_cast<ServerEventLoop *>(server->data);
    if (status < 0) {
        ERR_LOG("Accept error:{}", uv_strerror(status));
        return;
    }
    auto *client = new uv_tcp_t;
    int ret = uv_tcp_init(self->uv_loop(), client);
    if (ret != 0) {
        ERR_LOG("uv_tcp_init failed: {}", uv_err_name(ret));
        delete client;
        return;
    }
    // uv_accept 要求 server/client 在同一 loop：先在 boss loop 上完成 accept，
    // 再通过 ipc pipe + uv_write2 把句柄传递给 worker（libuv 标准跨 loop 传句柄）
    ret = uv_accept(server, (uv_stream_t *) client);
    if (ret != 0) {
        ERR_LOG("uv_accept failed: {}", uv_err_name(ret));
        uv_close((uv_handle_t *) client, [](uv_handle_t *h) { delete (uv_tcp_t *) h; });
        return;
    }
    self->handoffToWorker(client);
}

void ServerEventLoop::handoffToWorker(uv_tcp_t *client) {
    // round-robin 选择一个已接入的 worker 会话 pipe
    for (int tried = 0; tried < worker_num_; tried++) {
        uv_pipe_t *session = sessions_[next_worker_];
        next_worker_ = (next_worker_ + 1) % (size_t) worker_num_;
        if (session == nullptr) {
            continue;
        }
        auto *req = new uv_write_t;
        req->data = client;
        static char dummy = 0;
        uv_buf_t buf = uv_buf_init(&dummy, 1);
        int ret = uv_write2(req, (uv_stream_t *) session, &buf, 1, (uv_stream_t *) client, after_handoff_cb);
        if (ret == 0) {
            return; // 句柄已交出，boss 侧副本在 after_handoff_cb 中关闭
        }
        ERR_LOG("uv_write2 handoff failed: {}", uv_err_name(ret));
        delete req;
        break;
    }
    ERR_LOG("no ready worker session, drop connection");
    uv_close((uv_handle_t *) client, [](uv_handle_t *h) { delete (uv_tcp_t *) h; });
}

void ServerEventLoop::after_handoff_cb(uv_write_t *req, int status) {
    auto *client = static_cast<uv_tcp_t *>(req->data);
    if (status < 0) {
        // 派发失败：worker 不会收到该连接，关闭 boss 侧副本即可
        ERR_LOG("handoff write2 failed: {}", uv_strerror(status));
    }
    uv_close((uv_handle_t *) client, [](uv_handle_t *h) { delete (uv_tcp_t *) h; });
    delete req;
}

// boss：worker 的 ipc pipe 接入
void ServerEventLoop::uv_on_pipe_connection(uv_stream_t *server, int status) {
    auto *self = static_cast<ServerEventLoop *>(server->data);
    if (status < 0) {
        ERR_LOG("ipc accept error:{}", uv_strerror(status));
        return;
    }
    int idx = self->indexOfPipeServer((uv_pipe_t *) server);
    if (idx < 0) {
        return;
    }
    auto *session = new uv_pipe_t;
    int ret = uv_pipe_init(self->uv_loop(), session, 1);
    if (ret != 0) {
        ERR_LOG("uv_pipe_init session failed: {}", uv_err_name(ret));
        delete session;
        return;
    }
    ret = uv_accept(server, (uv_stream_t *) session);
    if (ret != 0) {
        ERR_LOG("ipc uv_accept failed: {}", uv_err_name(ret));
        uv_close((uv_handle_t *) session, [](uv_handle_t *h) { delete (uv_pipe_t *) h; });
        return;
    }
    self->sessions_[idx] = session;
    self->ready_workers_.fetch_add(1);
    {
        std::lock_guard<std::mutex> lk(self->ready_mtx_);
        self->ready_cv_.notify_all();
    }
    INFO_LOG("--- worker#{} ipc session established", idx);
}

void ServerEventLoop::onStopTriggered() {
    if (listening_ && !uv_is_closing((uv_handle_t *) &server_)) {
        uv_close((uv_handle_t *) &server_, nullptr);
    }
    for (auto &ps: pipe_servers_) {
        if (!uv_is_closing((uv_handle_t *) &ps)) {
            uv_close((uv_handle_t *) &ps, nullptr);
        }
    }
    for (auto *s: sessions_) {
        if (s != nullptr && !uv_is_closing((uv_handle_t *) s)) {
            uv_close((uv_handle_t *) s, [](uv_handle_t *h) { delete (uv_pipe_t *) h; });
        }
    }
    EventLoop::onStopTriggered();
}

} // namespace transport
