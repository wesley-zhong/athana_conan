//
// Created by zhongweiqi on 2026/1/13.
//

#ifndef ATHENA_DISCOVERY_H
#define ATHENA_DISCOVERY_H
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "AthenaEtcdClient.h"
#include "core/common/NodeInfo.h"


class AthenaDiscovery {
public:
    AthenaDiscovery(AthenaEtcdClient *client) {
        this->client = client;
    }

    void registerServer(std::shared_ptr<NodeInfo> nodeInfo);

    void keepAlive(const std::string &key, const std::string &myName);

    void watchKeys(const std::vector<std::string> &keysm,
                   std::function<void(const std::string_view &, const std::string_view &)> watchKeysCB);

    std::vector<NodeInfo*> getServerNode(const std::string& key);

    ~AthenaDiscovery() {
        delete client;
    }

private:
    AthenaEtcdClient *client;
    std::shared_ptr<NodeInfo> mySelf;
};

#endif //ATHENA_DISCOVERY_H
