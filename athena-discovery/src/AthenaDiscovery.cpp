//
// Created by zhongweiqi on 2026/1/13.
//

#include "AthenaDiscovery.h"
#include "JsonUtils.h"
#include "core/utils/NetUtils.h"

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