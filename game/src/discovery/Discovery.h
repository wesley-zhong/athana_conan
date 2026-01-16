//
// Created by zhongweiqi on 2026/1/16.
//

#ifndef DISCOVERY_H
#define  DISCOVERY_H
#include "AthenaDiscovery.h"
#include "core/common/AthenaConfig.h"

class Discovery {
public:
    static bool initWithConf(AthenaConfig &conf);

    static void onWatchKeyChange(const std::string_view &key, const std::string_view &value);

    static bool registerMySelf(const std::string &ip, int port, std::string sever_name, std::string value);

    static AthenaDiscovery *athena_discovery;
};


#endif //ATHENA_DISCOVERY_H
