//
// Created by zhongweiqi on 2026/1/13.
//

#include "AthenaDiscovery.h"
#include "core/utils/JsonUtils.h"
#include "core/utils/NetUtils.h"
#include "core/log/XLog.h"


void AthenaDiscovery::keepAlive(const std::string &key, const std::string &value) {
    client->keepAlive(key, value);
}

void AthenaDiscovery::watchKeys(const std::vector<std::string> &keys,
                                std::function<void(const std::string_view &, const std::string_view &)> watchKeysCB) {
    client->watchKeys(keys, watchKeysCB);
}

void AthenaDiscovery::registerServer(std::shared_ptr<NodeInfo> nodeInfo) {
    std::string localIp = NetUtils::getLocalIPs()[0];
    nodeInfo->service_id = nodeInfo->service_name + "/" + localIp + ":" + std::to_string(nodeInfo->port);
    std::string jsonStr = JsonUtils::SerializeNodeInfo(nodeInfo.get());

    keepAlive(nodeInfo->service_id, jsonStr);
}

std::vector<NodeInfo *> AthenaDiscovery::getServerNode(const std::string &key) {
    std::map<std::string, std::string> keyValues = client->getPrefix(key);
    std::vector<NodeInfo *> nodeVec;
    for (const auto &[key, value]: keyValues) {
        NodeInfo *nodeInfo = new NodeInfo();
        bool ret = JsonUtils::DeserializeNodeInfo(value, *nodeInfo);
        INFO_LOG("++++++++++++  GET KEY ={}  value ={}", key, value);
        nodeVec.push_back(nodeInfo);
    }
    return nodeVec;
}