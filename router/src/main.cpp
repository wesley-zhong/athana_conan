//
// Created by zhongweiqi on 2026/9/23.
//

#include <chrono>
#include <iostream>
#include <filesystem>
#include <csignal>
#include "log/XLog.h"
#include "transport/Dispatcher.h"

#include "ProtoInner.pb.h"
#include "common/AthenaConfig.h"

#include "utils/Snowflake.h"
#include "discovery/Discovery.h"
#include "network/RouterServerNetWorkHandler.h"
#include "transport/AthenaTcpServer.h"

#if defined(_WIN32)

#else
#include <unistd.h>
#endif

#include "transport/TcpClient.h"
#include "network/RouterClientNetWorkHandler.h"

static std::atomic<bool> g_running(true);
static std::condition_variable g_cv;
static std::mutex g_mutex;

void handleSignal(int signum)
{
    INFO_LOG("Received signal {} exiting...", signum);
    g_running = false;
    g_cv.notify_all(); // 唤醒主线程
}

int main(int argc, char** argv)
{
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
    core::xLogInitLog(core::LogLevel::LL_INFO, "../logs/router.log");

    std::filesystem::path cur_path = std::filesystem::current_path();
    INFO_LOG("+++  cur path: {}", cur_path.string());
    bool success = core::AthenaConfig::instance().load("config/router.toml");
    if (!success)
    {
        ERR_LOG("config ={} load failed", cur_path.string() + "/config/router.toml");
        return -1;
    }
    auto &config = core::AthenaConfig::instance();

    // snowflake init
    success = core::Snowflake::init(config.get("server", "worker-id", 0));
    if (!success)
    {
        ERR_LOG("Snowflake init failed");
        return -2;
    }
    INFO_LOG("Snowflake init ok");

    //tcp client
    RouterClientNetWorkHandler::initAllMsgRegister();
    RouterClientNetWorkHandler::startLogicThread(config.get("client", "logic-thread", 2));
    transport::TcpClient tcp_client;
    tcp_client.onConnected = RouterClientNetWorkHandler::onNewConnect;
    tcp_client.onClosed = RouterClientNetWorkHandler::onClosed;
    tcp_client.onRead = RouterClientNetWorkHandler::onMsg;
    tcp_client.onTriggerEvent = RouterClientNetWorkHandler::onEventTrigger;
    tcp_client.setChannelIdleTime(config.get("client", "idle-write-time", 3000),
                                  config.get("client", "idle-read-time", 9000));

    tcp_client.start();

    success = Discovery::initWithConf(core::AthenaConfig::instance(), tcp_client);
    if (!success)
    {
        ERR_LOG("initWithConf  faild");
        return -3;
    }

    int serverPort = config.get("server", "tcp-port", 0);
    INFO_LOG("#### bind server port:{}", serverPort);
    // tcp server
    RouterServerNetWorkHandler::initAllMsgRegister();
    RouterServerNetWorkHandler::startLogicThread(config.get("server", "io-thread", 1),
                                                 config.get("server", "logic-thread", 2));
    transport::AthenaTcpServer tcp_server;

    tcp_server.onNewConnection = RouterServerNetWorkHandler::onConnect;
    tcp_server.onRead = RouterServerNetWorkHandler::onMsg;
    tcp_server.onClosed = RouterServerNetWorkHandler::onClosed;
    tcp_server.onEventTrigger = RouterServerNetWorkHandler::onEventTrigger;

    tcp_server.setChannelIdleTime(config.get("server", "idle-read-time", 9000),
                                  config.get("server", "idle-write-time", 3000));
    if (!tcp_server.bind(serverPort).start(config.get("server", "event-loop-num", 2)))
    {
        ERR_LOG("tcp server start failed, port ={}", serverPort);
        return -4;
    }


    // 💡 主线程阻塞等待，无限期休眠（CPU 占用≈0）
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.wait(lock, [] { return !g_running.load(); });
    }

    INFO_LOG("service exited");
    return 0;
}
