//
// Created by zhongweiqi on 2025/10/23.
//

#include "AthenaTcpServer.h"
#include "Channel.h"
#include "core/log/XLog.h"

void AthenaTcpServer::uv_on_new_connection(uv_stream_t *server, int status) {
    INFO_LOG("--- uv_on_new_connection accept  new socket");
    AthenaTcpServer *tcpserver = (AthenaTcpServer *) server->data;
    tcpserver->onAccept(server, status);
}


void AthenaTcpServer::onAccept(uv_stream_t *server, int status) {
    if (status < 0) {
        ERR_LOG("Accept error:{}", uv_strerror(status));
        return;
    }
    INFO_LOG("event loop  accept  new socket");
    // accept into temporary uv_tcp_t to get the fd
    uv_tcp_t *client = new uv_tcp_t;
    uv_tcp_init(&main_loop, client);
    if (uv_accept(server, reinterpret_cast<uv_stream_t *>(client)) != 0) {
        uv_close(reinterpret_cast<uv_handle_t *>(client), [](uv_handle_t *h) { free(h); });
        return;
    }
    uv_os_sock_t sock;
    int ret = uv_fileno((const uv_handle_t *) client, (uv_os_fd_t *) &sock);
    if (ret != 0) {
        ERR_LOG(" uv_file no error ret ={}", ret);
        uv_close((uv_handle_t *) client, [](uv_handle_t *h) { free(h); });
        return;
    }

    // choose reactor
    const int idx = next_reactor++ % event_loops_.size();
    std::unique_ptr<EventLoop> &loopUptr = event_loops_[idx];
    loopUptr->asyncAccept(sock);

    //  延迟关闭主 loop 的临时 client
    uv_timer_t *timer = new uv_timer_t;
    uv_timer_init(uv_default_loop(), timer);
    timer->data = client;
    uv_timer_start(timer, [](uv_timer_t *t) {
        uv_close((uv_handle_t *) t->data, [](uv_handle_t *h) { delete (uv_tcp_t *) h; });
        uv_close((uv_handle_t *) t, [](uv_handle_t *h) { delete (uv_timer_t *) h; });
    }, 100, 0);
}

void AthenaTcpServer::start(int eventLoopNum) {
    for (int i = 0; i < eventLoopNum; i++) {
        auto it = std::make_unique<EventLoop>(this, event_trigger);
        event_loops_.push_back(std::move(it));
    }
    for (int i = 0; i < eventLoopNum; i++) {
        event_loops_[i]->start();
    }
    INFO_LOG("server  start io thread count ={}", eventLoopNum);
    uv_run(&main_loop, UV_RUN_DEFAULT);
}

AthenaTcpServer &AthenaTcpServer::bind(int port) {
    // 主Reactor监听
    uv_loop_init(&main_loop);
    uv_tcp_init(&main_loop, &server);
    server.data = this;

    sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    uv_tcp_bind(&server, reinterpret_cast<const sockaddr *>(&addr), 0);
    int ret = uv_listen((uv_stream_t *) &server, 128, uv_on_new_connection);
    if (ret != 0) {
        ERR_LOG(" listen failed: {}", ret);
    }
    INFO_LOG("#### server  bind socket port ={}", port);
    return *this;
}

void AthenaTcpServer::triggerEvent(Channel *channel, TriggerEventEnum reason) {
    if (onEventTrigger != nullptr) {
        onEventTrigger(channel, reason);
    }
}

//only surport one
AthenaTcpServer &AthenaTcpServer::setChannelIdleTime(uint64 idle_write_time, uint64 idle_read_time) {
    event_trigger = new IdleStateHandler(this, idle_write_time, idle_read_time);
    return *this;
}
