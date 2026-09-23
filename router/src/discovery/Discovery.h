//
// Created by zhongweiqi on 2026/9/23.
//

#ifndef DISCOVERY_H
#define  DISCOVERY_H

#include "discovery/AthenaDiscovery.h"
#include "common/AthenaConfig.h"
#include "transport/TcpClient.h"

class Discovery {
public:
    static bool initWithConf(const core::AthenaConfig &conf);

    static void onWatchKeyChange(etcd::Event::EventType eventType, const std::string_view &key, const std::string_view &value);

};


#endif //DISCOVERY_H
