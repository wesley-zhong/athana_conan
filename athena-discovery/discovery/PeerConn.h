//
// Created by zhongweiqi on 2026/2/9.
//

#ifndef ATHENA_PEERCONN_H
#define ATHENA_PEERCONN_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "common/NodeInfo.h"
#include "transport/Channel.h"

namespace discovery {

struct NodeChannelInfo {
    std::unique_ptr<core::NodeInfo> nodeInfo;
    // channel 可能从任意业务线程使用，统一用 shared_ptr 持有；
    // channel 关闭时由各服务 onClosed 回调 removeNodeChannel 摘除
    std::vector<std::shared_ptr<transport::Channel> > channels;
};

class PeerConn {
public:
    static void saveNode(std::unique_ptr<core::NodeInfo> nodeInfo);

    static void removeNode(const std::string &nodeKey);

    static void saveNodeChannel(const std::string &serviceId, transport::Channel *channel);

    // 连接关闭时摘除登记，避免 getRandomChannel 返回已关闭的 channel
    static void removeNodeChannel(transport::Channel *channel);

    static std::shared_ptr<transport::Channel> getRandomChannel(const std::string &serviceId);

    // 按 service_id 定点取 channel（router 转发场景：目标节点已由调用方算出）
    static std::shared_ptr<transport::Channel> getChannelByServiceId(const std::string &serviceId);

    // 对某类型下全部节点广播（router 转发场景：目标 player 不在本 router 管辖，扩散给同类节点）
    static std::vector<std::shared_ptr<transport::Channel>> getChannelsByType(int serverType);


    static bool sendMsg(std::shared_ptr<transport::Channel> channel, int msgId,
                        std::shared_ptr<google::protobuf::Message> msg);

    static bool sendMsg(int serverType, int msgId, std::shared_ptr<google::protobuf::Message> msg);


private:
    // 全部接口可能被网络线程与业务线程并发调用
    static std::mutex mutex_;
    static std::unordered_map<std::string, std::shared_ptr<NodeChannelInfo >> node_id_nodes;
    static std::unordered_map<int, std::vector<std::shared_ptr<NodeChannelInfo>>> node_type_nodes;
};


} // namespace discovery

#endif //ATHENA_PEERCONN_H
