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
#include "transport/AthenaTcpServer.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "transport/TcpClient.h"
#include "network/ClientNetWorkHandler.h"

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

    xLogInitLog(LogLevel::LL_INFO, "../logs/client.log");

    ClientNetWorkHandler::initAllMsgRegister();
    ClientNetWorkHandler::startThread(2);
    TcpClient tcp_client;
    tcp_client.setChannelIdleTime(5000, 0);
    tcp_client.onConnected = ClientNetWorkHandler::onConnect;
    tcp_client.onRead = ClientNetWorkHandler::onMsg;
    tcp_client.onTriggerEvent = ClientNetWorkHandler::onEventTrigger;

    tcp_client.start();

    //only wait for client event thread start finished
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tcp_client.connect("127.0.0.1", 6666);


    // AthenaTcpClient athena_tcp_client;
    // for (int i = 0; i < 2; ++i) {
    //     int connRet = athena_tcp_client.connect("localhost", 38881);
    //     if (connRet) {
    //         ERR_LOG(" errror  connect rest ={}", connRet);
    //     }
    // }
    // 💡 主线程阻塞等待，无限期休眠（CPU 占用≈0）
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.wait(lock, [] { return !g_running.load(); });
    }

    INFO_LOG("service exited");

    return 0;
}
