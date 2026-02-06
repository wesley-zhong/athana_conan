//
// Created by zhongweiqi on 2026/1/16.
//

#ifndef DISCOVERY_H
#define  DISCOVERY_H
#include "AthenaDiscovery.h"
#include "core/common/AthenaConfig.h"
#include "transport/TcpClient.h"

class Discovery {
public:
    static bool initWithConf(AthenaConfig &conf, TcpClient& tcpClient);

    static void onWatchKeyChange(const std::string_view &key, const std::string_view &value);

    static AthenaDiscovery *athena_discovery;
};


#endif //ATHENA_DISCOVERY_H
