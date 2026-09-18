//
// Created by zhongweiqi on 2026/2/9.
//

#ifndef ATHENA_PEERCONN_H
#define ATHENA_PEERCONN_H

#include <string>
#include <unordered_map>
#include <vector>
#include "common/NodeInfo.h"
#include "transport/Channel.h"

namespace discovery {

struct NodeChannelInfo {
    std::unique_ptr<core::NodeInfo> nodeInfo;
    std::vector<transport::Channel *> channels;
};

class PeerConn {
public:
    static void saveNode(std::unique_ptr<core::NodeInfo> nodeInfo);

    static void removeNode(const std::string &nodeKey);

    static void saveNodeChannel(const std::string &serviceId, transport::Channel *channel);

    transport::Channel *getRandomChannel(const std::string &srviceId);


    static bool sendMsg(transport::Channel *channel, int msgId, google::protobuf::Message *msg);

    static bool sendMsg(int serverType, int msgId, google::protobuf::Message *msg);


private:
    static std::unordered_map<std::string, std::shared_ptr<NodeChannelInfo >> node_id_nodes;
    static std::unordered_map<int, std::vector<std::shared_ptr<NodeChannelInfo>>> node_type_nodes;
};


} // namespace discovery

#endif //ATHENA_PEERCONN_H
