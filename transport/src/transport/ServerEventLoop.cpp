//
// Created by zhongweiqi on 2026/1/9.
//

#include "ServerEventLoop.h"

void uv_on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        ERR_LOG("Accept error:{}", uv_strerror(status));
        return;
    }
    INFO_LOG("--- uv_on_new_connection accept  new socket");
    EventLoop *event_loop = (EventLoop *) server->data;

    uv_tcp_t *client = new uv_tcp_t;
    uv_tcp_init(event_loop->uv_loop(), client);
    if (uv_accept(server, reinterpret_cast<uv_stream_t *>(client)) != 0) {
        uv_close(reinterpret_cast<uv_handle_t *>(client), [](uv_handle_t *h) { delete(h); });
        return;
    }

    uv_os_sock_t sock;
    int ret = uv_fileno((const uv_handle_t *) client, (uv_os_fd_t *) &sock);
    if (ret != 0) {
        ERR_LOG(" uv_file no error ret ={}", ret);
        uv_close((uv_handle_t *) client, [](uv_handle_t *h) { delete(h); });
        return;
    }


    uv_read_start((uv_stream_t *) client, EventLoop::uv_alloc_cb, EventLoop::uv_read_cb);
    Channel *channel = new Channel(event_loop, client, sock);
    client->data = channel;
    event_loop->onNewConnection(channel);
    INFO_LOG(" =================== START READ read data ");
}


void ServerEventLoop::bind(int port) {
    bindPort = port;
}


void ServerEventLoop::run() {
    initAsynEvent();
    uv_tcp_init(_loop, &server);
    server.data = this;
    sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", bindPort, &addr);


    uv_tcp_bind(&server, reinterpret_cast<const sockaddr *>(&addr), 0);
    int ret = uv_listen((uv_stream_t *) &server, 128, uv_on_new_connection);
    if (ret != 0) {
        ERR_LOG(" listen failed: {}", uv_err_name(ret));
    }
    INFO_LOG("#### server  bind socket port ={}", bindPort);
    doRun();
}
