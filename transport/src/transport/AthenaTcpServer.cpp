//
// Created by zhongweiqi on 2025/10/23.
//

#include "AthenaTcpServer.h"
#include "Channel.h"
#include "ServerEventLoop.h"
#include "core/log/XLog.h"



void AthenaTcpServer::start(int eventLoopNum) {
    for (int i = 0; i < eventLoopNum; i++) {
        auto it = std::make_shared<ServerEventLoop>(this, event_trigger);
        event_loops_.push_back(it);
    }
    for (int i = 0; i < eventLoopNum; i++) {
        event_loops_[i]->bind(bindPort);
        event_loops_[i]->start();
    }
}

AthenaTcpServer &AthenaTcpServer::bind(int port) {
    // 主Reactor监听
    this->bindPort = port;
    return *this;
}

void AthenaTcpServer::triggerEvent(Channel *channel, TriggerEventEnum reason) {
    if (onEventTrigger != nullptr) {
        onEventTrigger(channel, reason);
    }
}

//only support one
AthenaTcpServer &AthenaTcpServer::setChannelIdleTime(uint64 idle_write_time, uint64 idle_read_time) {
    event_trigger = new IdleStateHandler(this, idle_write_time, idle_read_time);
    return *this;
}
