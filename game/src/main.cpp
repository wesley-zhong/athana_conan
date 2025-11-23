#include <chrono>
#include "sol/sol.hpp"
#include <iostream>
#include <csignal>
#include "core/common/RingBuffer.hpp"
#include "core/log/XLog.h"
#include "objs/Player.h"
#include "core/common/ObjectPool.hpp"
#include "db/Dal.hpp"

#if defined(_WIN32)
#include <windows.h>
#else

#include <unistd.h>

#endif

#include "transport/AthenaTcpServer.h"

#include "network/GameServerNetWorkHandler.h"


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
    xLogInitLog(LogLevel::LL_INFO, "../logs/game.log");


    // init all functions call
    // GameServerNetWorkHandler::initAllMsgRegister();
    // GameServerNetWorkHandler::startLogicThread(3);

    //start server
    // AthenaTcpServer tcp_server;
    // tcp_server.setChannelIdleTime(5000, 0);
    // tcp_server.onNewConnection = GameServerNetWorkHandler::onNewConnect;
    // tcp_server.onRead = GameServerNetWorkHandler::onMsg;
    // tcp_server.onClosed = GameServerNetWorkHandler::onClosed;
    // tcp_server.onEventTrigger = GameServerNetWorkHandler::onEventTrigger;
    //
    // tcp_server.bind(9999).start(3);


    // connect db
    std::string ip = "127.0.0.1";
    Dal::Cache::init(ip, 6379, "", "", "");
    RedisResult redisResult;
    Dal::Cache::execute(&redisResult,"set ol:100064913 889");
    RedisResult redisResult1;
    Dal::Cache::execute(&redisResult1, "get ol:100064913");
    INFO_LOG("OUT STRING ={}",redisResult1.getStream());

//    Dal::DB::init(ip,3306,"gm_tool", "root","MyUN#FoyT!EtLnh7");
//    MysqlResult db_result;
//    Dal::DB::execute(&db_result, "select * from  user");



    INFO_LOG("==========================  wait release");
    std::this_thread::sleep_for(std::chrono::seconds(5));



    // 💡 主线程阻塞等待，无限期休眠（CPU 占用≈0）
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.wait(lock, [] { return !g_running.load(); });
    }

    INFO_LOG("service exited");

    return 0;
}
