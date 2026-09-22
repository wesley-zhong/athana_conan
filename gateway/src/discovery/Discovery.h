//
// Created by zhongweiqi on 2026/1/16.
//

#ifndef DISCOVERY_H
#define  DISCOVERY_H

#include "discovery/AthenaDiscovery.h"
#include "common/AthenaConfig.h"
#include "transport/TcpClient.h"

class Discovery {
public:
    static bool initWithConf(const core::AthenaConfig &conf, const transport::TcpClient &tcpClient);

    static void onWatchKeyChange(etcd::Event::EventType eventType, const std::string_view &key, const std::string_view &value);
private:
    const static  transport::TcpClient* ownTcpClient;
};


#endif //ATHENA_DISCOVERY_H
