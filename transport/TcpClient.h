//
// Created by zhongweiqi on 2025/10/27.
//

#ifndef ATHENA_TCPCLIENT_H
#define ATHENA_TCPCLIENT_H
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "transport/EventLoop.h"
#include "NetInterface.h"

namespace transport {


// netty 风格的多 reactor client：N 个 EventLoop 平摊全部出站连接。
// connect() 以 round-robin 选定 loop，该连接的读写/心跳/关闭都在这个 loop 线程完成，
// 业务回调可能来自任意 loop 线程，跨线程持有 channel 请用 EventLoop::channelPtr。
class TcpClient : public NetInterface {
public:
    TcpClient();

    ~TcpClient() override;

    // eventLoopNum = reactor 线程数，<1 时取 1；须在 setChannelIdleTime 之后调用
    void start(int eventLoopNum = 1);

    // 优雅停机：关闭所有连接，等待全部 loop 线程退出；幂等
    void stop();

    void on_new_connection(Channel *channel) override {
    }

    // 线程安全：round-robin 挑一个 loop 发起连接；须在 start() 之后、stop() 之前调用
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
    // start 后只读，stop 时清空；connect 与 stop 不可并发
    std::vector<std::unique_ptr<EventLoop>> loops;
    EventTrigger *event_trigger = nullptr;
    mutable std::atomic<size_t> next_loop{0};
};


} // namespace transport

#endif //ATHENA_TCPCLIENT_H
