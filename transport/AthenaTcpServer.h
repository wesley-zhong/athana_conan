//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_TCPSERVER_H
#define ATHENA_TCPSERVER_H
#include <memory>
#include <vector>
#include "EventLoop.h"

#include "NetInterface.h"
#include "Channel.h"
#include "EventDefs.h"
#include "ServerEventLoop.h"

namespace transport {

// netty 风格的多 reactor server：1 个 boss(ServerEventLoop) 负责监听/accept，
// N 个 worker(EventLoop) 负责连接的读写，连接通过 ipc pipe 由 boss 派发到 worker。
class AthenaTcpServer : public NetInterface {
public:
    AthenaTcpServer() = default;

    ~AthenaTcpServer() override;

    AthenaTcpServer &bind(int port);

    // eventLoopNum = worker 数量（不含 boss）
    // 返回 false 表示启动失败（worker 就绪超时或业务端口 bind/listen 失败），
    // 失败时内部已停止全部 loop，调用方应退出进程而非继续对外提供服务
    bool start(int eventLoopNum);

    // 优雅停机：关闭所有连接与监听，等待全部 loop 线程退出；幂等
    void stop();

    // channel 是否仍存活（channelPtr 拿到 shared_ptr 前的快速校验；
    // 对可能已释放的裸指针调用仍有风险，跨线程持有请用 channelPtr）
    bool isLive(Channel *channel) {
        return channel != nullptr && channel->event_loop() != nullptr && channel->event_loop()->isLive(channel);
    }

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

    AthenaTcpServer &setChannelIdleTime(uint64 idle_read_time, uint64 idle_write_time);

    void triggerEvent(Channel *channel, TriggerEventEnum reason) override;

    std::function<void(Channel *)> onNewConnection;
    std::function<void(Channel *, char *, int)> onRead;
    std::function<void(Channel *)> onClosed;
    std::function<void(Channel *, TriggerEventEnum reason)> onEventTrigger;

private:
    std::shared_ptr<ServerEventLoop> boss_;
    std::vector<std::shared_ptr<EventLoop> > workers_;
    EventTrigger *event_trigger = nullptr;
    int bindPort = 0;
};


} // namespace transport

#endif //ATHENA_TCPSERVER_H
