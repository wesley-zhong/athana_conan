//
// Created by zhongweiqi on 2026/1/16.
//

#include "Discovery.h"
#include "core/log/XLog.h"
#include "discovery/AthenaEtcdClient.h"
#include "core/utils/NetUtils.h"
#include "discovery/PeerConn.h"

bool Discovery::initWithConf(core::AthenaConfig &conf, transport::TcpClient &tcpClient) {

    std::shared_ptr<core::NodeInfo> myself = std::make_shared<core::NodeInfo>();
    myself->service_name = conf.get<std::string>("server", "name", std::string("None"));
    myself->port = conf.get<int>("server", "tcp-port", 8080);
    myself->ip = core::NetUtils::getLocalIPs()[0];
    myself->type = conf.get<int>("server", "type", 0);
    discovery::AthenaDiscovery::Instance()->setMySelfInfo(myself);


    std::string discoverAddrs = conf.get<std::string>("discover", "server_nodes", "http://127.0.0.1:2379");

    discovery::AthenaEtcdClient *etcd_client = new discovery::AthenaEtcdClient(discoverAddrs);
    int erro = etcd_client->connect();
    if (erro) {
        ERR_LOG("connect etcd {} failed , erro ={}", discoverAddrs, erro);
        return false;
    }
    discovery::AthenaDiscovery::Instance()->setEtcdClient(etcd_client);
 //   discovery::AthenaDiscovery::Instance() = new discovery::AthenaDiscovery(etcd_client);
    std::vector<std::string> watchKeys = conf.getArray<std::string>("discover", "watch-servers");
    if (!watchKeys.empty()) {
        discovery::AthenaDiscovery::Instance()->watchKeys(watchKeys, Discovery::onWatchKeyChange);
        for (auto key: watchKeys) {
            auto serverNodes = discovery::AthenaDiscovery::Instance()->getServerNode(key);
            if (serverNodes.empty()) {
                continue;
            }
            for (auto &node: serverNodes) {
                tcpClient.connect(node->ip, node->port);
                discovery::PeerConn::saveNode(std::move(node));
            }
        }
    }

    discovery::AthenaDiscovery::Instance()->registerServer();
    return true;
}

void Discovery::onWatchKeyChange(const std::string_view &key, const std::string_view &value) {
    INFO_LOG("================= on watched key ={} value ={}", key, value);
}


