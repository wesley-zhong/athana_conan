//
// Created by zhongweiqi on 2026/2/9.
//

#include "PeerConn.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "common/RandomUtil.h"

namespace discovery {

std::mutex PeerConn::mutex_;
std::unordered_map<std::string, std::shared_ptr<NodeChannelInfo>> PeerConn::node_id_nodes;
std::unordered_map<int, std::vector<std::shared_ptr<NodeChannelInfo>>> PeerConn::node_type_nodes;

void PeerConn::saveNode(std::unique_ptr<core::NodeInfo> nodeInfo) {
    if (!nodeInfo) return;

    const auto &serviceId = nodeInfo->service_id;
    const auto nodeType = nodeInfo->type;

    // 2. 使用 try_emplace 进行一次查找并尝试插入
    // 只有在 key 不存在时才会执行构造和插入，避免了先 find 再 insert 的双重开销
    std::lock_guard<std::mutex> lk(mutex_);
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
    std::lock_guard<std::mutex> lk(mutex_);
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

void PeerConn::saveNodeChannel(const std::string &serviceId, transport::Channel *channel) {
    if (serviceId.empty() || channel == nullptr) {
        return;
    }
    // 从 loop 注册表取 shared_ptr 持有，避免登记已释放的裸指针
    std::shared_ptr<transport::Channel> shared = channel->event_loop()
                                                     ? channel->event_loop()->channelPtr(channel)
                                                     : nullptr;
    if (shared == nullptr) {
        WARN_LOG("saveNodeChannel failed: channel not live, serviceId ={}", serviceId);
        return;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    // 1. 查找对应的节点信息
    auto it = node_id_nodes.find(serviceId);
    if (it == node_id_nodes.end()) {
        WARN_LOG("saveNodeChannel failed: serviceId {} not found", serviceId);
        return;
    }

    auto &nodeChannelInfo = it->second;

    // 2. 检查该 channel 是否已经存在于 vector 中（防止重复添加）
    auto &vec = nodeChannelInfo->channels;
    auto channelIt = std::find(vec.begin(), vec.end(), shared);

    if (channelIt == vec.end()) {
        // 3. 不存在则添加
        vec.push_back(std::move(shared));
        INFO_LOG("Channel added to serviceId: {}, total channels: {}",
                 serviceId, vec.size());
    } else {
        INFO_LOG("Channel already exists for serviceId: {}", serviceId);
    }
}

void PeerConn::removeNodeChannel(transport::Channel *channel) {
    if (channel == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto &kv: node_id_nodes) {
        auto &vec = kv.second->channels;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [channel](const std::shared_ptr<transport::Channel> &c) {
                                     return c.get() == channel;
                                 }), vec.end());
    }
}

std::shared_ptr<transport::Channel> PeerConn::getRandomChannel(const std::string &serviceId) {
    std::lock_guard<std::mutex> lk(mutex_);
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
    // uniform_int_distribution 闭区间：上界必须是 size-1，否则越界
    int32 randomIndex = core::RandomUtil::getInt(0, (int32) channels.size() - 1);

    return channels[randomIndex];
}

std::shared_ptr<transport::Channel> PeerConn::getChannelByServiceId(const std::string &serviceId) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = node_id_nodes.find(serviceId);
    if (it == node_id_nodes.end()) {
        return nullptr;
    }
    auto &channels = it->second->channels;
    // 取该节点最近一条可用连接；已关闭的连接由 onClosed -> removeNodeChannel 摘除
    for (auto rit = channels.rbegin(); rit != channels.rend(); ++rit) {
        if (!(*rit)->isClosed()) {
            return *rit;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<transport::Channel>> PeerConn::getChannelsByType(int serverType) {
    std::vector<std::shared_ptr<transport::Channel>> result;
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = node_type_nodes.find(serverType);
    if (it == node_type_nodes.end()) {
        return result;
    }
    for (auto &node: it->second) {
        for (auto &channel: node->channels) {
            // 快速过滤已关闭的连接，避免拿到马上要失效的 channel
            if (!channel->isClosed()) {
                result.push_back(channel);
            }
        }
    }
    return result;
}

bool PeerConn::sendMsg(int serverType, int msgId, std::shared_ptr<google::protobuf::Message> msg) {
    std::shared_ptr<transport::Channel> channel;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = node_type_nodes.find(serverType);
        if (it == node_type_nodes.end() || it->second.empty()) {
            WARN_LOG("sendMsg failed: no node of type ={}", serverType);
            return false;
        }
        //TODO 负载均衡
        auto &node = it->second[0];
        if (node->channels.empty()) {
            WARN_LOG("sendMsg failed: node type ={} has no channel", serverType);
            return false;
        }
        channel = node->channels[0];
    }
    return sendMsg(std::move(channel), msgId, std::move(msg));

}

bool PeerConn::sendMsg(std::shared_ptr<transport::Channel> channel, int msgId,
                       std::shared_ptr<google::protobuf::Message> msg) {
    if (channel == nullptr || msg == nullptr) {
        return false;
    }
    channel->sendMsg(msgId, std::move(msg));
    return true;
}

} // namespace discovery
