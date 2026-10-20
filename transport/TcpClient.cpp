//
// Created by zhongweiqi on 2025/10/27.
//

#include "TcpClient.h"

namespace transport {


TcpClient::TcpClient() {
}

TcpClient::~TcpClient() {
    stop();
}


void TcpClient::connect(const std::string &ip, int port) const {
    // start 前后语义不同：启动前 loops 为空直接丢弃（与旧版 loop 未建就 connect 一致），
    // 启动后 next_loop 只增不减，无锁 round-robin 安全
    if (loops.empty()) {
        return;
    }
    size_t idx = next_loop.fetch_add(1, std::memory_order_relaxed) % loops.size();
    loops[idx]->asyncConnect(ip, port);
}

void TcpClient::start(int eventLoopNum) {
    if (eventLoopNum < 1) {
        eventLoopNum = 1;
    }
    loops.reserve(eventLoopNum);
    for (int i = 0; i < eventLoopNum; i++) {
        // asyncs 在 loop 线程初始化，与 AthenaTcpServer 相同：先建后 start，
        // 避免提前 wake 的未定义行为
        auto &loop = loops.emplace_back(std::make_unique<EventLoop>(this, event_trigger.get()));
        loop->setBuffSize(channel_recv_buff_size_, channel_send_buff_size_);
    }
    for (auto &loop: loops) {
        loop->start();
    }
}

void TcpClient::stop() {
    // 先全部请求停机再统一 join，与 AthenaTcpServer::stop 顺序一致
    for (auto &loop: loops) {
        loop->stop();
    }
    for (auto &loop: loops) {
        loop->join();
    }
    loops.clear();
    next_loop.store(0, std::memory_order_relaxed);
}


void TcpClient::triggerEvent(Channel *channel, TriggerEventEnum reason) {
    if (onTriggerEvent != nullptr) {
        onTriggerEvent(channel, reason);
    }
}

//only surport one
TcpClient &TcpClient::setChannelIdleTime(uint64 idle_write_time, uint64 idle_read_time) {
    event_trigger = std::make_unique<IdleStateHandler>(this, idle_write_time, idle_read_time);
    return *this;
}

TcpClient &TcpClient::setChannelBuffSize(size_t recvBuffSize, size_t sendBuffSize) {
    // start 前暂存，start() 创建 loop 时下发
    channel_recv_buff_size_ = recvBuffSize;
    channel_send_buff_size_ = sendBuffSize;
    return *this;
}

} // namespace transport
