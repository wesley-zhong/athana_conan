//
// Created by zhongweiqi on 2026/1/13.
//

#include "AthenaDiscovery.h"

void AthenaDiscovery::keepAlive(const std::string &key, const std::string &value) {
    client->keepAlive(key, value);
}

void AthenaDiscovery::watchKeys(const std::vector<std::string> &keys,
                                std::function<void(const std::string_view &, const std::string_view &)> watchKeysCB) {
    client->watchKeys(keys, watchKeysCB);
}
