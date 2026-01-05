//
// Created by zhongweiqi on 2025/10/20.
//

#include <chrono>
#include <iostream>
#include <csignal>
#include "core/common/RingBuffer.hpp"
#include "core/log/XLog.h"
#include "transport/Dispatcher.h"

#include "ProtoInner.pb.h"

#include "thread/AthenaThreadPool.h"
#include "core/common/ObjectPool.hpp"
#include "db/Dal.hpp"
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

    //tcp client
    GateClientNetWorkHandler::initAllMsgRegister();
    GateClientNetWorkHandler::startLogicThread(2);
    TcpClient tcp_client;
    tcp_client.setChannelIdleTime(3000, 15000);
    tcp_client.onConnected = GateClientNetWorkHandler::onNewConnect;
    tcp_client.onRead = GateClientNetWorkHandler::onMsg;
    tcp_client.onTriggerEvent = GateClientNetWorkHandler::onEventTrigger;
    tcp_client.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tcp_client.connect("127.0.0.1", 9999);



    // tcp server
    GatewayServerNetWorkHandler::initAllMsgRegister();
    GatewayServerNetWorkHandler::startLogicThread(2);
    AthenaTcpServer tcp_server;

    tcp_server.onNewConnection = GatewayServerNetWorkHandler::onConnect;
    tcp_server.onRead = GatewayServerNetWorkHandler::onMsg;
    tcp_server.onClosed = GatewayServerNetWorkHandler::onClosed;
    tcp_server.onEventTrigger = GatewayServerNetWorkHandler::onEventTrigger;

    tcp_server.setChannelIdleTime(5000, 0);
    tcp_server.bind(6666).start(4);


    // 💡 主线程阻塞等待，无限期休眠（CPU 占用≈0）
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.wait(lock, [] { return !g_running.load(); });
    }

    INFO_LOG("service exited");

    return 0;
}
