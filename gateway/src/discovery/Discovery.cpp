//
// Created by zhongweiqi on 2026/1/16.
//

#include "Discovery.h"
#include "core/log/XLog.h"
#include "AthenaEtcdClient.h"
#include "core/utils/NetUtils.h"
#include "network/GateClientNetWorkHandler.h"

bool Discovery::initWithConf(AthenaConfig &conf) {
    std::string discoverAddrs = conf.get<std::string>("discover", "server_nodes", "http://127.0.0.1:2379");
    int myPort = conf.get<int>("server", "tcp-por", 8080);
    std::string myName = conf.get<std::string>("server", "name", std::string("None"));
    AthenaEtcdClient *etcd_client = new AthenaEtcdClient(discoverAddrs);
    int erro = etcd_client->connect();
    if (erro) {
        ERR_LOG("connect etcd {} failed , erro ={}", discoverAddrs, erro);
        return false;
    }
    athena_discovery = new AthenaDiscovery(etcd_client);
    std::vector<std::string> watchKeys = conf.getArray<std::string>("discover", "watch-servers");
    if (!watchKeys.empty()) {
        initTcpClient();
        athena_discovery->watchKeys(watchKeys, Discovery::onWatchKeyChange);
    }
    registerMySelf(NetUtils::getLocalIPs()[0], myPort, myName, "hello");
    return true;
}

void Discovery::onWatchKeyChange(const std::string_view &key, const std::string_view &value) {
    INFO_LOG("================= on watched key ={} value ={}", key, value);
    //tcp_client->connect()
}

bool Discovery::registerMySelf(const std::string &ip, int port, std::string sever_name, std::string value) {
    std::string key = sever_name + "/" + ip + ":" + std::to_string(port);

    std::string body = key + ":" + value;
    athena_discovery->keepAlive(key, body);
    return true;
}

void Discovery::initTcpClient() {
    tcp_client = new TcpClient();
    tcp_client->setChannelIdleTime(3000, 15000);
    tcp_client->onConnected = GateClientNetWorkHandler::onNewConnect;
    tcp_client->onRead = GateClientNetWorkHandler::onMsg;
    tcp_client->onTriggerEvent = GateClientNetWorkHandler::onEventTrigger;
    tcp_client->start();
}

AthenaDiscovery *Discovery::athena_discovery = nullptr;
TcpClient *Discovery::tcp_client = nullptr;
