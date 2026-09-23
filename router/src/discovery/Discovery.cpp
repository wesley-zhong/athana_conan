//
// Created by zhongweiqi on 2026/9/23.
//

#include "Discovery.h"
#include "log/XLog.h"
#include "discovery/AthenaEtcdClient.h"
#include "utils/NetUtils.h"
#include "discovery/PeerConn.h"
#include "core/common/NodeInfo.h"

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
    discovery::AthenaDiscovery::Instance()->registerServer();
    return true;
}

void Discovery::onWatchKeyChange(etcd::Event::EventType eventType, const std::string_view& key,
                                 const std::string_view& value)
{
    INFO_LOG("================= on watched key ={} value ={} event={} ", key, value, static_cast<int>(eventType));
    auto watchedServer = core::AthenaConfig::instance().getArray<std::string>("discover", "watch-servers");
    if (eventType == etcd::Event::EventType::PUT)
    {
        INFO_LOG("++++++++++++  GET KEY ={}  value ={} ", key, value);
        return;
    }

    if (eventType == etcd::Event::EventType::DELETE_)
    {
        if (const auto it = std::ranges::find(watchedServer, key); it != watchedServer.end())
        {
            // 节点下线：等 etcd lease 过期摘除 key；连接由对端关闭/心跳超时回收
        }
    }
}