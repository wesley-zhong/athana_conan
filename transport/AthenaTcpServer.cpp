//
// Created by zhongweiqi on 2025/10/23.
//

#include "AthenaTcpServer.h"
#include "Channel.h"
#include "ServerEventLoop.h"
#include <chrono>
#include <thread>
#include "log/XLog.h"

namespace transport {


bool AthenaTcpServer::start(int eventLoopNum) {
    if (eventLoopNum < 1) {
        eventLoopNum = 1;
    }
    if (bindPort <= 0) {
        ERR_LOG("bind port not set (or invalid): {}", bindPort);
        return false;
    }
    // 1. 先启动 boss：建立各 worker 的 ipc pipe server
    boss_ = std::make_shared<ServerEventLoop>(this, event_trigger, eventLoopNum);
    boss_->bind(bindPort);
    boss_->start();

    // 2. 启动 worker 并接入 boss 的 ipc pipe
    workers_.reserve(eventLoopNum);
    for (int i = 0; i < eventLoopNum; i++) {
        workers_.push_back(std::make_shared<EventLoop>(this, event_trigger));
    }
    for (auto &w: workers_) {
        w->start();
    }
    // 必须在 start() 之后再 connectIpc：asyncs 在 loop 线程初始化，提前 wake 是未定义行为。
    // 每个 worker 只连一次：ipc_pipe_ 是 EventLoop 的单成员，重复 init/connect 会覆盖
    // 在途句柄（UB），boss 侧还会 accept 出第二个 session 覆盖 sessions_[i] 并泄漏旧的
    for (int i = 0; i < eventLoopNum; i++) {
        workers_[i]->connectIpc(boss_->pipeName(i));
    }

    // 3. 等待全部 worker 会话就绪后再监听业务端口。
    // 超时说明派发通道没建好，此时开监听只会 accept 后全部丢弃（黑洞端口），
    // 直接判启动失败，不降级运行
    if (!boss_->waitWorkersReady(5000)) {
        ERR_LOG("wait workers ready timeout, expect {} workers, start failed", eventLoopNum);
        stop();
        return false;
    }
    boss_->startListen();
    // bind/listen 在 boss loop 线程执行，稍候 listen_ok_；失败则停止并上报
    for (int i = 0; i < 100; i++) {
        if (boss_->listeningOk()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (i == 99) {
            ERR_LOG("listen not ready after 1s, start failed (see bind/listen error above)");
        }
    }
    if (!boss_->listeningOk()) {
        stop();
        return false;
    }
    INFO_LOG("#### AthenaTcpServer started: boss + {} workers, port ={}", eventLoopNum, bindPort);
    return true;
}

void AthenaTcpServer::stop() {
    if (boss_ != nullptr) {
        boss_->stop();
    }
    for (auto &w: workers_) {
        w->stop();
    }
    if (boss_ != nullptr) {
        boss_->join();
        boss_.reset();
    }
    for (auto &w: workers_) {
        w->join();
    }
    workers_.clear();
}

AthenaTcpServer::~AthenaTcpServer() {
    stop();
}

AthenaTcpServer &AthenaTcpServer::bind(int port) {
    this->bindPort = port;
    return *this;
}

void AthenaTcpServer::triggerEvent(Channel *channel, TriggerEventEnum reason) {
    if (onEventTrigger != nullptr) {
        onEventTrigger(channel, reason);
    }
}

//only support one
AthenaTcpServer &AthenaTcpServer::setChannelIdleTime(uint64 idle_read_time , uint64 idle_write_time) {
    event_trigger = new IdleStateHandler(this, idle_write_time, idle_read_time);
    return *this;
}

} // namespace transport
