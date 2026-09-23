//
// Created by zhongweiqi on 2025/10/20.
//

#include <chrono>
#include <iostream>
#include <filesystem>
#include <csignal>
#include "common/RingBuffer.hpp"
#include "log/XLog.h"
#include "transport/Dispatcher.h"

#include "ProtoInner.pb.h"

#include "common/ObjectPool.hpp"
#include "dal/Dal.hpp"
#include "transport/AthenaTcpServer.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "common/AthenaConfig.h"
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

    core::xLogInitLog(core::LogLevel::LL_INFO, "../logs/client.log");

    std::filesystem::path cur_path = std::filesystem::current_path();
    bool success = core::AthenaConfig::instance().load("config/client.toml");
    if (!success)
    {
        ERR_LOG("config ={} load failed", cur_path.string() + "/config/client.toml");
        return -1;
    }
    auto &config = core::AthenaConfig::instance();

    ClientNetWorkHandler::initAllMsgRegister();
    ClientNetWorkHandler::startThread(config.get("client", "logic-thread", 2));
    transport::TcpClient tcp_client;
    tcp_client.setChannelIdleTime(config.get("client", "idle-write-time", 3000),
                                  config.get("client", "idle-read-time", 5000));
    tcp_client.onConnected = ClientNetWorkHandler::onConnect;
    tcp_client.onRead = ClientNetWorkHandler::onMsg;
    tcp_client.onTriggerEvent = ClientNetWorkHandler::onEventTrigger;

    tcp_client.start();


    //only wait for client event thread start finished
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   // tcp_client.connect("127.0.0.1", 6666);


    // AthenaTcpClient athena_tcp_client;
    std::string serverIp = config.getString("server", "ip", "127.0.0.1");
    int serverPort = config.get("server", "port", 0);
    int connectCount = config.get("server", "connect-count", 1);
    for (int i = 0; i < connectCount; ++i) {
        tcp_client.connect(serverIp, serverPort);
    }
    // 💡 主线程阻塞等待，无限期休眠（CPU 占用≈0）
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.wait(lock, [] { return !g_running.load(); });
    }

    INFO_LOG("service exited");

    return 0;
}
