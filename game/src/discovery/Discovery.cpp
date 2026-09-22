//
// Created by zhongweiqi on 2026/1/16.
//

#include "Discovery.h"
#include "log/XLog.h"
#include "discovery/AthenaEtcdClient.h"
#include "utils/NetUtils.h"
#include "discovery/PeerConn.h"

bool Discovery::initWithConf(const core::AthenaConfig& conf)
{
    const auto myself = std::make_shared<core::NodeInfo>();
    myself->service_name = conf.get<std::string>("server", "name", std::string("None"));
    myself->port = conf.get<int>("server", "tcp-port", 8080);
    myself->ip = core::NetUtils::getLocalIPs()[0];
    myself->type = conf.get<int>("server", "type", 0);
    discovery::AthenaDiscovery::Instance()->setMySelfInfo(myself);

    auto discoverAddrs = conf.get<std::string>("discover", "server_nodes", "http://127.0.0.1:2379");
    auto* etcd_client = new discovery::AthenaEtcdClient(discoverAddrs);
    if (int err = etcd_client->connect(); err)
    {
        ERR_LOG("connect etcd {} failed, err ={}", discoverAddrs, err);
        return false;
    }
    discovery::AthenaDiscovery::Instance()->setEtcdClient(etcd_client);
    if (auto watchKeys = conf.getArray<std::string>("discover", "watch-servers"); !watchKeys.empty())
    {
        discovery::AthenaDiscovery::Instance()->watchKeys(watchKeys, Discovery::onWatchKeyChange);
        for (const auto& key : watchKeys)
        {
            auto serverNodes = discovery::AthenaDiscovery::Instance()->getServerNode(key);
            if (serverNodes.empty())
            {
                continue;
            }
            for (auto& node : serverNodes)
            {
                discovery::PeerConn::saveNode(std::move(node));
            }
        }
    }

    discovery::AthenaDiscovery::Instance()->registerServer();
    return true;
}

void Discovery::onWatchKeyChange(etcd::Event::EventType eventType, const std::string_view& key,
                                 const std::string_view& value)
{
    INFO_LOG("================= on watched key ={} value ={}", key, value);
}
