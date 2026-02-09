//
// Created by zhongweiqi on 2026/1/16.
//

#include "Discovery.h"
#include "core/log/XLog.h"
#include "AthenaEtcdClient.h"
#include "core/utils/NetUtils.h"
#include "PeerConn.h"

bool Discovery::initWithConf(AthenaConfig &conf) {
    std::string discoverAddrs = conf.get<std::string>("discover", "server_nodes", "http://127.0.0.1:2379");


    AthenaEtcdClient *etcd_client = new AthenaEtcdClient(discoverAddrs);
    int erro = etcd_client->connect();
    if (erro) {
        ERR_LOG("connect etcd {} failed , erro ={}", discoverAddrs, erro);
        return false;
    }
    athena_discovery = new AthenaDiscovery(etcd_client);
    std::vector<std::string> watchKeys = conf.getArray<std::string>("discover", "watch-servers");
    if (!watchKeys.empty()) {
        athena_discovery->watchKeys(watchKeys, Discovery::onWatchKeyChange);
        for (auto key: watchKeys) {
            auto serverNodes = athena_discovery->getServerNode(key);
            if (serverNodes.empty()) {
                continue;
            }
            for (auto &node: serverNodes) {
                PeerConn::saveNode(std::move(node));
            }
        }
    }


    std::shared_ptr<NodeInfo> nodeInfo = std::make_shared<NodeInfo>();
    nodeInfo->service_name = conf.get<std::string>("server", "name", std::string("None"));
    nodeInfo->port = conf.get<int>("server", "tcp-port", 8080);
    nodeInfo->ip = NetUtils::getLocalIPs()[0];
    nodeInfo->type = conf.get<int>("server", "type", 0);

    athena_discovery->registerServer(nodeInfo);
    return true;
}

void Discovery::onWatchKeyChange(const std::string_view &key, const std::string_view &value) {
    INFO_LOG("================= on watched key ={} value ={}", key, value);
}


AthenaDiscovery *Discovery::athena_discovery = nullptr;
