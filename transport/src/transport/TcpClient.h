//
// Created by zhongweiqi on 2025/10/27.
//

#ifndef ATHENA_TCPCLIENT_H
#define ATHENA_TCPCLIENT_H
#include <string>
#include "transport/EventLoop.h"
#include "NetInterface.h"


class TcpClient : public NetInterface {
public:
    TcpClient();

    ~TcpClient() {
    }

    void start();

    void on_new_connection(Channel *channel) override {
    }

    void connect(const std::string &ip, int port) const;

    void on_connected(Channel *channel, int status) override {
        if (onConnected != nullptr) {
            onConnected(channel, status);
        }
    }

    void on_read(Channel *channel, char *body, int len) override {
        if (onRead != nullptr) {
            onRead(channel, body, len);
        }
    }

    void on_closed(Channel *channel) override {
        if (onClosed != nullptr) {
            onClosed(channel);
        }
    }

    TcpClient& setChannelIdleTime(uint64 idle_write_time, uint64 idle_read_time);

    void triggerEvent(Channel *channel, TriggerEventEnum reason) override;

    std::function<void(Channel *, int)> onConnected;
    std::function<void(Channel *, char *, int)> onRead;
    std::function<void(Channel *)> onClosed;
    std::function<void(Channel *, TriggerEventEnum reason)> onTriggerEvent;

private:
    EventLoop *loop;
    EventTrigger *event_trigger;
};


#endif //ATHENA_TCPCLIENT_H
