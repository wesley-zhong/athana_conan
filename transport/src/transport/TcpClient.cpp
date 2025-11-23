//
// Created by zhongweiqi on 2025/10/27.
//

#include "TcpClient.h"


TcpClient::TcpClient() {
}


void TcpClient::connect(const std::string &ip, int port) const {
    loop->asyncConnect(ip, port);
}

void TcpClient::start() {
    loop = new EventLoop(this, event_trigger);
    loop->start();
}


void TcpClient::triggerEvent(Channel *channel, TriggerEventEnum reason) {
    if (onTriggerEvent != nullptr) {
        onTriggerEvent(channel, reason);
    }
}

//only surport one
TcpClient &TcpClient::setChannelIdleTime(uint64 idle_write_time, uint64 idle_read_time) {
    event_trigger = new IdleStateHandler(this, idle_write_time, idle_read_time);
    return *this;
}
