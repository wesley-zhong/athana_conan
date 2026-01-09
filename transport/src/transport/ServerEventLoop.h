//
// Created by zhongweiqi on 2026/1/9.
//

#ifndef ATHENA_SERVEREVENTLOOP_H
#define ATHENA_SERVEREVENTLOOP_H
#include "EventLoop.h"

class ServerEventLoop : public EventLoop {
public:
    ServerEventLoop(NetInterface *tcpInterFace, EventTrigger *event_trigger) : EventLoop(tcpInterFace, event_trigger) {
    }

    ~ServerEventLoop() {
    }

    void bind(int port);


     void run() override;

private:
    uv_tcp_t server;
    int bindPort;
};


#endif //ATHENA_SERVEREVENTLOOP_H
