//
// Created by zhongweiqi on 2026/2/9.
//

#include "PeerConn.h"
#include "core/log/XLog.h"
#include "core/common/RandomUtil.h"

void PeerConn::saveNode(std::unique_ptr<NodeInfo> nodeInfo) {
    if (!nodeInfo) return;

    const auto &serviceId = nodeInfo->service_id;
    const auto nodeType = nodeInfo->type;

    // 2. 使用 try_emplace 进行一次查找并尝试插入
    // 只有在 key 不存在时才会执行构造和插入，避免了先 find 再 insert 的双重开销
    auto [idIt, inserted] = node_id_nodes.try_emplace(serviceId, nullptr);

    if (!inserted) {
        INFO_LOG("service id ={} already exist", serviceId);
        return;
    }

    // 3. 确认为新节点后，再构造 shared_ptr
    auto uNode = std::make_shared<NodeChannelInfo>();
    uNode->nodeInfo = std::move(nodeInfo);

    // 更新刚才 try_emplace 留下的空位置
    idIt->second = uNode;

    // 4. 修复原有的 Bug：无论类型是否存在，都必须 push_back
    // operator[] 如果 key 不存在会默认构造一个空的 vector
    node_type_nodes[nodeType].push_back(std::move(uNode));
}

void PeerConn::removeNode(const std::string &nodeKey) {
// 1. 先在 ID 映射表中查找
    auto itId = node_id_nodes.find(nodeKey);
    if (itId == node_id_nodes.end()) {
        return; // 节点不存在，直接返回
    }

    // 2. 获取节点信息（由于后面要 erase，先提出来）
    auto targetNode = itId->second;
    int nodeType = targetNode->nodeInfo->type;

    // 3. 从类型映射表的 vector 中移除
    auto itType = node_type_nodes.find(nodeType);
    if (itType != node_type_nodes.end()) {
        auto &vec = itType->second;

        // 使用 std::remove_if 结合 erase (Erase-Remove Idiom)
        // 比较指针地址，确保删除的是同一个对象
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [&targetNode](const std::shared_ptr<NodeChannelInfo> &n) {
                                     return n == targetNode;
                                 }), vec.end());

        // 可选：如果该类型的 vector 空了，可以把整个 key 删掉节省空间
        if (vec.empty()) {
            node_type_nodes.erase(itType);
        }
    }

    // 4. 最后从 ID 映射表中删除
    node_id_nodes.erase(itId);
    INFO_LOG("Successfully removed node: {}", nodeKey);
}

void PeerConn::saveNodeChannel(const std::string &serviceId, Channel *channel) {
    if (serviceId.empty() || channel == nullptr) {
        return;
    }

    // 1. 查找对应的节点信息
    auto it = node_id_nodes.find(serviceId);
    if (it == node_id_nodes.end()) {
        WARN_LOG("saveNodeChannel failed: serviceId {} not found", serviceId);
        return;
    }

    auto &nodeChannelInfo = it->second;

    // 2. 检查该 channel 是否已经存在于 vector 中（防止重复添加）
    auto &vec = nodeChannelInfo->channels;
    auto channelIt = std::find(vec.begin(), vec.end(), channel);

    if (channelIt == vec.end()) {
        // 3. 不存在则添加
        vec.push_back(channel);
        INFO_LOG("Channel added to serviceId: {}, total channels: {}",
                 serviceId, vec.size());
    } else {
        INFO_LOG("Channel already exists for serviceId: {}", serviceId);
    }
}

Channel *PeerConn::getRandomChannel(const std::string &serviceId) {
    // 1. 查找节点
    auto it = node_id_nodes.find(serviceId);
    if (it == node_id_nodes.end()) {
        return nullptr;
    }

    auto &channels = it->second->channels;

    // 2. 检查通道列表是否为空
    if (channels.empty()) {
        return nullptr;
    }

    // 3. 如果只有一个通道，直接返回，省去随机计算
    if (channels.size() == 1) {
        return channels[0];
    }

    // 4. 生成随机索引
    // 使用 thread_local 保证随机数引擎在线程间安全且只初始化一次

    int32 randomIndex = RandomUtil::getInt(0, channels.size());

    return channels[randomIndex];
}

std::unordered_map<std::string, std::shared_ptr<NodeChannelInfo>> PeerConn::node_id_nodes;
std::unordered_map<int, std::vector<std::shared_ptr<NodeChannelInfo>>> PeerConn::node_type_nodes;

bool PeerConn::sendMsg(int serverType, int msgId, google::protobuf::Message *msg) {
    auto it = node_type_nodes.find(serverType);
    if (it == node_type_nodes.end()) {
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
