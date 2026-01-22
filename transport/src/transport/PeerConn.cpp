//
// Created by zhongweiqi on 2026/1/22.
//

#include "PeerConn.h"


bool PeerConn::sendMsg(int serverType, int msgId, google::protobuf::Message *msg) {
    auto it = serverTypeNodes.find(serverType);
    if (it == serverTypeNodes.end()) {
        return false;
    }
    //TODO
    auto channel = it->second[0]->channels[0];
    return sendMsg(channel, msgId, msg);

}

bool PeerConn::sendMsg(Channel *channel, int msgId, google::protobuf::Message *msg) {
    channel->sendMsg(msgId, msg);
    return true;
}

void PeerConn::remove(const std::string &serviceId) {

}

void PeerConn::saveNodeChannel(NodeInfo *nodeInfo, Channel *channel) {
    auto it = serviceChanel.find(nodeInfo->service_id);
    auto nodeChannel = new NodeCannel();
    if (it == serviceChanel.end()) {
        serviceChanel[nodeInfo->service_id] = nodeChannel;
        nodeChannel->channels.push_back(channel);
    }
    it->second->channels.push_back(channel);

    auto serverTypeIter = serverTypeNodes.find(nodeInfo->type);
    if (serverTypeIter == serverTypeNodes.end()) {
        serverTypeNodes[nodeInfo->type] = std::vector<NodeCannel *>();
    }
    serverTypeNodes[nodeInfo->type].push_back(nodeChannel);
}