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
#include "core/common/Singleton.h"
#include "core/utils/NetUtils.h"


class AthenaDiscovery : public Singleton<AthenaDiscovery> {
public:
    AthenaDiscovery() {
    }

    void setEtcdClient(AthenaEtcdClient *client) {
        this->client = client;
    }

    void setMySelfInfo(std::shared_ptr<NodeInfo> me) {
        mySelf = me;
        std::string localIp = NetUtils::getLocalIPs()[0];
        mySelf->service_id = mySelf->service_name + "/" + localIp + ":" + std::to_string(mySelf->port);
    }

    void registerServer();

    void keepAlive(const std::string &key, const std::string &myName);

    void watchKeys(const std::vector<std::string> &keysm,
                   std::function<void(const std::string_view &, const std::string_view &)> watchKeysCB);

    std::vector<std::unique_ptr<NodeInfo >> getServerNode(const std::string &key);

    std::shared_ptr<NodeInfo> getMySelf() {
        return mySelf;
    }

    ~AthenaDiscovery() {
        delete client;
    }

private:
    AthenaEtcdClient *client;
    std::shared_ptr<NodeInfo> mySelf;
};

#endif //ATHENA_DISCOVERY_H
