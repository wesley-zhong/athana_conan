//
// Created by zhongweiqi on 2025/10/20.
//

#include <chrono>
#include <iostream>
#include <filesystem>
#include <csignal>
#include "core/common/RingBuffer.hpp"
#include "core/log/XLog.h"
#include "transport/Dispatcher.h"

#include "ProtoInner.pb.h"
#include "core/common/AthenaConfig.h"

#include "thread/AthenaThreadPool.h"
#include "core/common/ObjectPool.hpp"
#include "db/Dal.hpp"
#include "discovery/Discovery.h"
#include "network/GatewayServerNetWorkHandler.h"
#include "transport/AthenaTcpServer.h"

#if defined(_WIN32)

#include <windows.h>

#else
#include <unistd.h>
#endif

#include "transport/TcpClient.h"
#include "network/GateClientNetWorkHandler.h"

static std::atomic<bool> g_running(true);
static std::condition_variable g_cv;
static std::mutex g_mutex;

void handleSignal(int signum) {
    INFO_LOG("Received signal {} exiting...", signum);
    g_running = false;
    g_cv.notify_all(); // 唤醒主线程
}

int main(int argc, char **argv) {
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
    xLogInitLog(LogLevel::LL_INFO, "../logs/gateway.log");

    std::filesystem::path cur_path = std::filesystem::current_path();
    INFO_LOG("+++  cur path: {}", cur_path.string());
    bool success = AthenaConfig::instance().load("config/gateway.toml");
    if (!success) {
        ERR_LOG("config ={} load failed", cur_path.string() + "/config/gateway.toml");
        return -1;
    }

    //tcp client
    GateClientNetWorkHandler::initAllMsgRegister();
    GateClientNetWorkHandler::startLogicThread(2);
    TcpClient tcp_client;
    tcp_client.onConnected = GateClientNetWorkHandler::onNewConnect;
    tcp_client.onClosed = GateClientNetWorkHandler::onClosed;
    tcp_client.onRead = GateClientNetWorkHandler::onMsg;
    tcp_client.onTriggerEvent  = GateClientNetWorkHandler::onEventTrigger;
    tcp_client.setChannelIdleTime(5000, 3000);

    tcp_client.start();

    success = Discovery::initWithConf(AthenaConfig::instance(), tcp_client);
    if (!success) {
        ERR_LOG("initWithConf  faild");
        return -2;
    }

    int serverPort = AthenaConfig::instance().get("server", "tcp-port", 0);
    INFO_LOG("#### bind server port:{}", serverPort);
    // tcp server
    GatewayServerNetWorkHandler::initAllMsgRegister();
    GatewayServerNetWorkHandler::startLogicThread(2);
    AthenaTcpServer tcp_server;

    tcp_server.onNewConnection = GatewayServerNetWorkHandler::onConnect;
    tcp_server.onRead = GatewayServerNetWorkHandler::onMsg;
    tcp_server.onClosed = GatewayServerNetWorkHandler::onClosed;
    tcp_server.onEventTrigger = GatewayServerNetWorkHandler::onEventTrigger;

    tcp_server.setChannelIdleTime(5000, 0);
    tcp_server.bind(serverPort).start(1);



    // 💡 主线程阻塞等待，无限期休眠（CPU 占用≈0）
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.wait(lock, [] { return !g_running.load(); });
    }

    INFO_LOG("service exited");
    return 0;
}
