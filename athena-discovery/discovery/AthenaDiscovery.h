//
// Created by zhongweiqi on 2026/1/13.
//

#ifndef ATHENA_DISCOVERY_H
#define ATHENA_DISCOVERY_H

#include <string>
#include <vector>
#include <memory>
#include "AthenaEtcdClient.h"
#include "common/NodeInfo.h"
#include "common/Singleton.h"
#include "utils/NetUtils.h"

namespace discovery
{
    class AthenaDiscovery : public core::Singleton<AthenaDiscovery>
    {
    public:
        AthenaDiscovery()
        {
        }

        void setEtcdClient(AthenaEtcdClient* etcdClient)
        {
            this->client = etcdClient;
        }

        void setMySelfInfo(std::shared_ptr<core::NodeInfo> me)
        {
            mySelf = me;
            std::string localIp = core::NetUtils::getLocalIPs()[0];
            mySelf->service_id = mySelf->service_name + "/" + localIp + ":" + std::to_string(mySelf->port);
        }

        void registerServer();

        void keepAlive(const std::string& key, const std::string& myName);

        void watchKeys(const std::vector<std::string>& keysm,
                       std::function<void(etcd::Event::EventType, const std::string_view&, const std::string_view&)>
                       watchKeysCB);

        std::vector<std::unique_ptr<core::NodeInfo>> getServerNode(const std::string& key);

        std::shared_ptr<core::NodeInfo> getMySelf()
        {
            return mySelf;
        }

        ~AthenaDiscovery()
        {
            delete client;
        }

    private:
        AthenaEtcdClient* client;
        std::shared_ptr<core::NodeInfo> mySelf;
    };
} // namespace discovery

#endif //ATHENA_DISCOVERY_H
