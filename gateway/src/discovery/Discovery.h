//
// Created by zhongweiqi on 2026/1/16.
//

#ifndef DISCOVERY_H
#define  DISCOVERY_H

#include "discovery/AthenaDiscovery.h"
#include "core/common/AthenaConfig.h"
#include "transport/TcpClient.h"

class Discovery {
public:
    static bool initWithConf(AthenaConfig &conf, TcpClient &tcpClient);

    static void onWatchKeyChange(const std::string_view &key, const std::string_view &value);
};


#endif //ATHENA_DISCOVERY_H
