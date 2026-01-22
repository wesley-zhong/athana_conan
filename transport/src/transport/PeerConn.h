//
// Created by zhongweiqi on 2026/1/22.
//

#ifndef ATHENA_PEERCONN_H
#define ATHENA_PEERCONN_H

#include <unordered_map>
#include<string>
#include<vector>
#include "core/common/NodeInfo.h"
#include "Channel.h"
#include "google/protobuf/message.h"

struct NodeCannel {
    NodeInfo *nodeInfo;
    std::vector<Channel *> channels;
};

class PeerConn {

public:

    static void remove(const std::string &key);

    static void saveNodeChannel(NodeInfo *nodeInfo, Channel *channel);

    static bool sendMsg(Channel *channel, int msgId, google::protobuf::Message *msg);

    static bool sendMsg(int serverType, int msgId, google::protobuf::Message *msg);


private:
    static std::unordered_map<std::string, NodeCannel *> serviceChanel; //key: service_id;
    static std::unordered_map<int32, std::vector<NodeCannel *>> serverTypeNodes;  //key : service type
};


#endif //ATHENA_PEERCONN_H
