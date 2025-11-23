//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_TCPSERVER_H
#define ATHENA_TCPSERVER_H
#include <atomic>
#include <memory>
#include <vector>
#include "EventLoop.h"

#include "NetInterface.h"
#include "Channel.h"
#include "EventDefs.h"

class AthenaTcpServer : public NetInterface {
public:
    AthenaTcpServer() {
    }

    ~AthenaTcpServer() {
    }


    AthenaTcpServer &bind(int port);

    void start(int eventLoopNum);

    void stop();

    void onAccept(uv_stream_t *server, int status);

    void on_connected(Channel *channel, int status) override {
    }

    void on_new_connection(Channel *channel) override {
        if (onNewConnection != nullptr) {
            onNewConnection(channel);
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

    AthenaTcpServer &setChannelIdleTime(uint64 idle_read_time, uint64 idle_write_time) ;

    void triggerEvent(Channel *channel, TriggerEventEnum reason) override;

    std::function<void(Channel *)> onNewConnection;
    std::function<void(Channel *, char *, int)> onRead;
    std::function<void(Channel *)> onClosed;
    std::function<void(Channel *, TriggerEventEnum reason)> onEventTrigger;

    static void uv_on_new_connection(uv_stream_t *server, int status);

private:
    std::atomic<size_t> next_reactor{0};
    std::vector<std::unique_ptr<EventLoop> > event_loops_;
    uv_loop_t main_loop;
    uv_tcp_t server;
    EventTrigger *event_trigger;
};


#endif //ATHENA_TCPSERVER_H
