//
// Created by zhongweiqi on 2026/1/22.
//

#ifndef ATHENA_NODEINFO_H
#define ATHENA_NODEINFO_H

#include <string>
#include <unordered_map>
#include <vector>

struct NodeInfo {
    std::string service_id;
    std::string service_name;
    std::string ip;
    int type;
    int port;
    int net_port;
    int id;
    int group_id;
    std::unordered_map<std::string, std::string> meta_data;
    int64_t keep_alive_lease_id;
};

#endif //ATHENA_NODEINFO_H
