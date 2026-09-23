//
// Created by zhongweiqi on 2025/10/23.
//

#include "EventLoop.h"
#include "log/XLog.h"
#include <string>
#include "AthenaTcpServer.h"
#include "IdleStateHandler.h"

namespace transport {

void async_accept_cb(uv_async_t *handler) {
    EventLoop *event_loop = static_cast<EventLoop *>(handler->data);
    event_loop->execute();
}

void async_connect_cb(uv_async_t *handler) {
    EventLoop *event_loop = static_cast<EventLoop *>(handler->data);
    event_loop->execute();
}

void async_write_cb(uv_async_t *handler) {
    EventLoop *event_loop = static_cast<EventLoop *>(handler->data);
    event_loop->execute();
}

void async_stop_cb(uv_async_t *handler) {
    EventLoop *event_loop = static_cast<EventLoop *>(handler->data);
    event_loop->onStopTriggered();
}

EventLoop::EventLoop(NetInterface *tcpInterFace, EventTrigger *event_trigger) : _netInterface(tcpInterFace) {
    maxPackBody = static_cast<char *>(malloc(8192));
    maxPackBodyLen = 8192;
    _eventTrigger = event_trigger;
    _loop = new uv_loop_t;
    running_ = false;
}

EventLoop::~EventLoop() {
    stop();
    join();
    if (maxPackBody != nullptr) {
        free(maxPackBody);
        maxPackBody = nullptr;
    }
    // 注册表销毁：未走完 close 序列的 channel 在此兜底析构（仅堆内存，uv 句柄已由
    // 停机流程/walk 兜底关闭）
    std::lock_guard<std::mutex> lk(live_mtx_);
    channels_.clear();
}

void EventLoop::uv_alloc_cb(uv_handle_t *h, size_t s, uv_buf_t *buf) {
    Channel *channel = (Channel *) h->data;
    size_t lineWritAbleLen = 0;
    uint8 *writePtr = channel->recv_buffer->linearWriteablePtr(&lineWritAbleLen);
    buf->base = (char *) writePtr;
    // uv_buf_t::len 在 Win32 是 ULONG、POSIX 是 size_t，跟随平台类型转换避免截断警告
    buf->len = static_cast<decltype(buf->len)>(lineWritAbleLen);
    if (lineWritAbleLen == 0) {
        // ring 已满且没有可解析的完整包：包长非法(超大包)或对端恶意刷数据，关闭连接
        ERR_LOG("recv ring full, close it addr ={}", channel->getAddr());
        // 提供 1 字节缓冲避免 libuv 对 len=0 的 alloc 产生误判，数据会随连接一起丢弃
        static char dummy = 0;
        buf->base = &dummy;
        buf->len = 1;
        channel->close();
    }
}

void EventLoop::uv_on_connect(uv_connect_t *req, int status) {
    Channel *channel = static_cast<Channel *>(req->data);
    EventLoop *event_loop = channel->event_loop();
    delete req;
    if (status < 0) {
        ERR_LOG("connect failed: {}, addr: {}", uv_strerror(status), channel->getAddr());
        event_loop->_netInterface->on_connected(channel, status);
        channel->close();
        return;
    }
    channel->refreshAddr();
    uv_read_start((uv_stream_t *) channel->client, uv_alloc_cb, uv_read_cb);
    event_loop->_netInterface->on_connected(channel, 0);
    event_loop->onNewConnection(channel);
}


void EventLoop::uv_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    Channel *channel = static_cast<Channel *>(client->data);
    channel->onRead(client, nread, buf);
    if (nread <= 0 || channel->isClosed()) {
        return;
    }

    EventLoop *event_loop = channel->event_loop();
    // get one complete packet
    while (!channel->isClosed()) {
        int packageLen = channel->peekNextPackLen();
        if (packageLen < 0) {
            if (packageLen == -2) {
                ERR_LOG("illegal packet len from {}, close it len ={}", channel->getAddr(), packageLen);
                channel->close();
            }
            return;
        }
        char *packBuf = event_loop->getPacketBuff(packageLen);
        if (packBuf == nullptr) {
            channel->close();
            return;
        }
        channel->getPack(packBuf, packageLen);
        event_loop->_netInterface->on_read(channel, packBuf, packageLen);
    }
}

void EventLoop::uv_on_timer(uv_timer_t *timer) {
    Channel *channel = (Channel *) timer->data;
    if (channel->isClosed()) {
        return;
    }
    EventLoop *event_loop = channel->event_loop();
    uint64_t now = uv_now(event_loop->uv_loop());
    event_loop->event_trigger()->onTimer(channel, now);
}

void EventLoop::asyncConnect(const std::string &ip, int port) {
    EventLoop *event_loop = this;
    executeOnEventLoop([ip, port, event_loop]() {
        INFO_LOG("------------- do connect idp ={} port ={}", ip, port);
        sockaddr_in dest;
        int ret = uv_ip4_addr(ip.c_str(), port, &dest);
        if (ret != 0) {
            ERR_LOG("uv_ip4_addr failed code =code {}:{}", ret, uv_strerror(ret));
            return;
        }
        auto *connect_req = new uv_connect_t;
        auto *client = new uv_tcp_t;
        ret = uv_tcp_init(event_loop->uv_loop(), client);
        if (ret != 0) {
            ERR_LOG("uv_tcp_init failed code ={}:{}", ret, uv_err_name(ret));
            delete client;
            delete connect_req;
            return;
        }
        auto channel = std::make_shared<Channel>(event_loop, client);
        client->data = channel.get();
        event_loop->registerChannel(channel);
        connect_req->data = channel.get();
        ret = uv_tcp_connect(connect_req, client, (const struct sockaddr *) &dest, uv_on_connect);
        if (ret != 0) {
            ERR_LOG("uv_tcp_connect failed code ={}:{}", ret, uv_strerror(ret));
            delete connect_req;
            event_loop->_netInterface->on_connected(channel.get(), ret);
            channel->close();
        }
    });
}

void EventLoop::onNewConnection(Channel *channel) {
    _netInterface->on_new_connection(channel);
    if (_eventTrigger != nullptr) {
        startHeartbeatTimer(channel);
    }
}

void EventLoop::onClosed(Channel *channel) const {
    _netInterface->on_closed(channel);
}

void EventLoop::onRead(Channel *channel, char *body, int len) const {
    _netInterface->on_read(channel, body, len);
}

char *EventLoop::getPacketBuff(int needLen) {
    if (needLen > maxPackBodyLen) {
        auto *newBuff = static_cast<char *>(realloc(maxPackBody, needLen));
        if (newBuff == nullptr) {
            ERR_LOG("grow pack buff failed, need ={}", needLen);
            return nullptr;
        }
        maxPackBody = newBuff;
        maxPackBodyLen = needLen;
    }
    return maxPackBody;
}

int EventLoop::initAsynEvent() {
    uv_loop_init(_loop);
    // store reactor pointer in loop->data for later retrieval in read callbacks
    _loop->data = this;

    int ret = uv_async_init(_loop, &uv_async_write, async_write_cb);
    if (ret != 0) {
        ERR_LOG("uv_async_init  async_write  ret ={} ", ret);
        return ret;
    }
    uv_async_write.data = this;
    ret = uv_async_init(_loop, &uv_async_accept, async_accept_cb);
    if (ret != 0) {
        ERR_LOG("uv_async_init  async_accept ret ={} ", ret);
        return ret;
    }
    uv_async_accept.data = this;

    ret = uv_async_init(_loop, &uv_async_connect, async_connect_cb);
    if (ret != 0) {
        ERR_LOG("uv_async_init  async_connect ret ={} ", ret);
        return ret;
    }
    uv_async_connect.data = this;

    ret = uv_async_init(_loop, &uv_async_stop, async_stop_cb);
    if (ret != 0) {
        ERR_LOG("uv_async_init  async_stop ret ={} ", ret);
        return ret;
    }
    uv_async_stop.data = this;
    asyncs_inited_.store(true, std::memory_order_release);
    return 0;
}


void EventLoop::run() {
    initAsynEvent();
    // 启动 libuv 事件循环
    doRun();
}

void EventLoop::doRun() {
    INFO_LOG("############## event loop started");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = true;
    }
    cv_.notify_all(); // 通知 start() 线程已经准备好了

    // uv_run 在引用计数归零（stop 流程关闭全部句柄）后返回
    uv_run(_loop, UV_RUN_DEFAULT);

    // 兜底：关闭一切残留句柄（正常停机已在 onStopTriggered 全部关闭）
    uv_walk(_loop, [](uv_handle_t *handle, void *) {
        if (!uv_is_closing(handle)) {
            uv_close(handle, nullptr);
        }
    }, nullptr);
    uv_run(_loop, UV_RUN_NOWAIT); // 让 close 回调跑完
    execute(); // 排干收尾任务（channel 兜底析构等）
    if (uv_loop_close(_loop) != 0) {
        WARN_LOG("uv_loop_close not clean, leaked handles in loop");
    }
    delete _loop;
    _loop = nullptr;
    INFO_LOG(" event  run end ");
}


void EventLoop::execute() {
    VOID_FUN task;
    while (pop(task)) {
        task();
    }
}


void EventLoop::start() {
    t = std::thread(&EventLoop::run, this);
    // 等待子线程开始执行
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return running_; });
}

void EventLoop::stop() {
    if (stopping_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    if (!asyncs_inited_.load(std::memory_order_acquire)) {
        // loop 从未启动，无需唤醒
        return;
    }
    uv_async_send(&uv_async_stop);
}

void EventLoop::join() {
    if (t.joinable()) {
        t.join();
    }
}

void EventLoop::onStopTriggered() {
    // 0. 关闭 worker 端 ipc pipe（boss 的会话先关时 read_cb 也会走到这里，幂等）
    closeIpcPipe();
    // 1. 关闭所有存活 channel（close 内部会入队 closeInLoop）
    std::vector<Channel *> snapshot;
    {
        std::lock_guard<std::mutex> lk(live_mtx_);
        snapshot.reserve(channels_.size());
        for (auto &kv: channels_) {
            snapshot.push_back(kv.first);
        }
    }
    for (Channel *ch: snapshot) {
        ch->close();
    }
    // 2. loop 线程内直接排干：closeInLoop 发起真正的 uv_close 并通知业务层
    execute();
    // 3. 关闭唤醒句柄，uv_run 引用计数归零后自然返回
    uv_close(reinterpret_cast<uv_handle_t *>(&uv_async_write), nullptr);
    uv_close(reinterpret_cast<uv_handle_t *>(&uv_async_accept), nullptr);
    uv_close(reinterpret_cast<uv_handle_t *>(&uv_async_connect), nullptr);
    uv_close(reinterpret_cast<uv_handle_t *>(&uv_async_stop), nullptr);
}


void EventLoop::startHeartbeatTimer(Channel *channel) {
    int erro = uv_timer_init(this->uv_loop(), channel->getTimer());
    if (erro != 0) {
        ERR_LOG("XXXXXXXXXX  uv_timer_init  erro ret ={}", erro);
        return;
    }
    channel->initPackTime();
    uint64 interval = _eventTrigger != nullptr ? _eventTrigger->timerIntervalMs() : 3000;
    erro = uv_timer_start(channel->getTimer(), uv_on_timer, interval, interval);
    if (erro != 0) {
        ERR_LOG("XXXXXXXXX  uv_timer_start erro ret ={}", erro);
    }
}

// ===== channel 注册表 =====

void EventLoop::registerChannel(std::shared_ptr<Channel> channel) {
    std::lock_guard<std::mutex> lk(live_mtx_);
    channels_[channel.get()] = std::move(channel);
}

std::shared_ptr<Channel> EventLoop::channelPtr(Channel *channel) {
    std::lock_guard<std::mutex> lk(live_mtx_);
    auto it = channels_.find(channel);
    if (it == channels_.end()) {
        return nullptr;
    }
    return it->second;
}

bool EventLoop::isLive(Channel *channel) {
    std::lock_guard<std::mutex> lk(live_mtx_);
    return channels_.count(channel) > 0;
}

void EventLoop::releaseChannel(Channel *channel) {
    // loop 线程：tcp/timer close 回调都完成后调用；最后一个 shared_ptr 在此析构
    std::lock_guard<std::mutex> lk(live_mtx_);
    channels_.erase(channel);
}

// ===== worker ipc（boss/worker 多 reactor 的句柄传递通道）=====

void EventLoop::connectIpc(const std::string &pipe_name) {
    ipc_pipe_name_ = pipe_name;
    executeOnEventLoop([this, pipe_name]() {
        int ret = uv_pipe_init(_loop, &ipc_pipe_, 1);
        if (ret != 0) {
            ERR_LOG("uv_pipe_init failed: {}", uv_err_name(ret));
            return;
        }
        ipc_pipe_.data = this;
        ipc_inited_ = true;
        ipc_connect_req_.data = this;
        uv_pipe_connect(&ipc_connect_req_, &ipc_pipe_, pipe_name.c_str(), uv_ipc_connect_cb);
    });
}

void EventLoop::uv_ipc_connect_cb(uv_connect_t *req, int status) {
    auto *self = static_cast<EventLoop *>(req->data);
    if (status < 0) {
        ERR_LOG("worker ipc connect to boss failed: {}", uv_strerror(status));
        return;
    }
    INFO_LOG("worker ipc pipe connected to boss");
    uv_read_start((uv_stream_t *) &self->ipc_pipe_, ipc_alloc_cb, uv_ipc_read_cb);
}

void EventLoop::ipc_alloc_cb(uv_handle_t *h, size_t s, uv_buf_t *buf) {
    auto *self = static_cast<EventLoop *>(h->data);
    // ipc pipe 只用于接收句柄，业务数据是 1 字节占位，读到临时缓冲丢弃即可
    buf->base = self->ipc_scratch_;
    buf->len = sizeof(self->ipc_scratch_);
}

void EventLoop::uv_ipc_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    auto *self = static_cast<EventLoop *>(stream->data);
    if (nread < 0) {
        // boss 侧停机/断开后必然走到这里：ipc pipe 是 active handle，
        // 必须关闭否则 uv_run 永不返回（AthenaTcpServer::stop 会挂死在 join）
        ERR_LOG("worker ipc pipe error: {}, closing", uv_strerror((int) nread));
        self->closeIpcPipe();
        return;
    }
    // boss 通过 uv_write2 传递过来的连接句柄，逐个采纳到本 loop
    while (uv_pipe_pending_count((uv_pipe_t *) stream) > 0) {
        auto *client = new uv_tcp_t;
        int ret = uv_tcp_init(self->uv_loop(), client);
        if (ret != 0) {
            ERR_LOG("uv_tcp_init on worker failed: {}", uv_err_name(ret));
            delete client;
            return;
        }
        ret = uv_accept(stream, (uv_stream_t *) client);
        if (ret != 0) {
            ERR_LOG("worker ipc accept failed: {}", uv_err_name(ret));
            uv_close((uv_handle_t *) client, [](uv_handle_t *h) { delete (uv_tcp_t *) h; });
            continue;
        }
        INFO_LOG("--- worker adopted new connection from boss");
        self->adoptConnection(client);
    }
}

void EventLoop::adoptConnection(uv_tcp_t *client) {
    auto channel = std::make_shared<Channel>(this, client);
    client->data = channel.get();
    registerChannel(channel);
    uv_read_start((uv_stream_t *) client, uv_alloc_cb, uv_read_cb);
    onNewConnection(channel.get());
}

void EventLoop::closeIpcPipe() {
    if (!ipc_inited_ || ipc_closing_) {
        return;
    }
    ipc_closing_ = true;
    uv_close((uv_handle_t *) &ipc_pipe_, nullptr);
}

} // namespace transport
