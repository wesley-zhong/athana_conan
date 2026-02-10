//
// Created by zhongweiqi on 2026/1/16.
//

#include "Discovery.h"
#include "core/log/XLog.h"
#include "discovery/AthenaEtcdClient.h"
#include "core/utils/NetUtils.h"
#include "discovery/PeerConn.h"

bool Discovery::initWithConf(AthenaConfig &conf) {

    std::shared_ptr<NodeInfo> myself = std::make_shared<NodeInfo>();
    myself->service_name = conf.get<std::string>("server", "name", std::string("None"));
    myself->port = conf.get<int>("server", "tcp-port", 8080);
    myself->ip = NetUtils::getLocalIPs()[0];
    myself->type = conf.get<int>("server", "type", 0);
    AthenaDiscovery::Instance()->setMySelfInfo(myself);


    std::string discoverAddrs = conf.get<std::string>("discover", "server_nodes", "http://127.0.0.1:2379");
    AthenaEtcdClient *etcd_client = new AthenaEtcdClient(discoverAddrs);
    int erro = etcd_client->connect();
    if (erro) {
        ERR_LOG("connect etcd {} failed , erro ={}", discoverAddrs, erro);
        return false;
    }
    AthenaDiscovery::Instance()->setEtcdClient(etcd_client);
    std::vector<std::string> watchKeys = conf.getArray<std::string>("discover", "watch-servers");
    if (!watchKeys.empty()) {
        AthenaDiscovery::Instance()->watchKeys(watchKeys, Discovery::onWatchKeyChange);
        for (auto key: watchKeys) {
            auto serverNodes = AthenaDiscovery::Instance()->getServerNode(key);
            if (serverNodes.empty()) {
                continue;
            }
            for (auto &node: serverNodes) {
                PeerConn::saveNode(std::move(node));
            }
        }
    }

    AthenaDiscovery::Instance()->registerServer();
    return true;
}

void Discovery::onWatchKeyChange(const std::string_view &key, const std::string_view &value) {
    INFO_LOG("================= on watched key ={} value ={}", key, value);
}

