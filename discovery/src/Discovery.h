//
// Created by zhongweiqi on 2026/1/13.
//

#ifndef ATHENA_DISCOVERY_H
#define ATHENA_DISCOVERY_H
#include <string>
#include <unordered_map>
#include <vector>

#include "AthenaEtcdClient.h"
#include "Register.h"

struct NodeInfo {
    std::string service_id;
    std::string service_name;
    int port;
    int ip;
    int net_port;
    int id;
    int group_id;
    std::unordered_map<std::string, std::string> meta_data;
    int64_t keep_alive_lease_id;
};

class Discovery {
public:
    Discovery(AthenaEtcdClient *client) {
        this->client = client;
    }

    void registerMyself(int myPort, int myType, std::string_view myName);

    void watchKeys(const std::vector<std::string> &keysm,  std::function<void(const std::string_view &, const std::string_view &)> watchKeysCB);

    ~Discovery() {
        delete client;
        delete register_;
        delete mySelf;
    }

private:
    Register *register_ = new Register();
    AthenaEtcdClient *client;
    NodeInfo *mySelf = new NodeInfo();
};

#endif //ATHENA_DISCOVERY_H
