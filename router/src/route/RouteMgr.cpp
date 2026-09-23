//
// Created by zhongweiqi on 2026/9/23.
//

#include "RouteMgr.h"
#include "log/XLog.h"
#include "transport/Dispatcher.h"
#include "discovery/PeerConn.h"
#include "common/NodeInfo.h"

std::mutex RouteMgr::mutex_;
std::unordered_map<int64, std::string> RouteMgr::player_routes_;

void RouteMgr::registMsg() {
    REGISTER_MSG_ID_FUN(INNER_ROUTE_TRANSFER, InnerRouteTransfer, onRouteTransfer);
}

void RouteMgr::bindPlayer(int64 playerId, const std::string &serviceId) {
    std::lock_guard<std::mutex> lk(mutex_);
    player_routes_[playerId] = serviceId;
}

void RouteMgr::unbindPlayer(int64 playerId) {
    std::lock_guard<std::mutex> lk(mutex_);
    player_routes_.erase(playerId);
}

std::string RouteMgr::findPlayerServiceId(int64 playerId) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = player_routes_.find(playerId);
    return it == player_routes_.end() ? std::string() : it->second;
}

void RouteMgr::onRouteTransfer(transport::Channel *channel, InnerRouteTransfer *msg) {
    // actor 闭包只带 shared_ptr/值语义成员，裸 channel 仅供日志取地址
    INFO_LOG("route transfer from ={} player ={} innerMsgId ={} bodyLen ={}",
             channel->getAddr(), msg->player_id(), msg->inner_msg_id(), (int) msg->body().size());

    // 1. 指定目标节点的直连转发
    if (!msg->dest_service_id().empty()) {
        if (forwardToService(msg->dest_service_id(), *msg)) {
            return;
        }
        ERR_LOG("route transfer to dest ={} failed, player ={}", msg->dest_service_id(), msg->player_id());
        return;
    }

    // 2. 按 player_id 查路由表定点转发
    std::string dest = findPlayerServiceId(msg->player_id());
    if (!dest.empty() && forwardToService(dest, *msg)) {
        return;
    }

    // 3. 表里查不到：对全部 game 节点扩散，由目标节点按 player 判断是否自己处理
    auto channels = discovery::PeerConn::getChannelsByType(core::SRV_TYPE_GAME);
    if (channels.empty()) {
        ERR_LOG("route transfer drop: no game node, player ={} innerMsgId ={}",
                msg->player_id(), msg->inner_msg_id());
        return;
    }
    for (auto &destChannel: channels) {
        destChannel->sendRawMsg(msg->inner_msg_id(), msg->body().data(), (int) msg->body().size());
    }
}

bool RouteMgr::forwardToService(const std::string &serviceId, const InnerRouteTransfer &msg) {
    auto channel = discovery::PeerConn::getChannelByServiceId(serviceId);
    if (channel == nullptr) {
        WARN_LOG("route forward: serviceId ={} has no live channel", serviceId);
        return false;
    }
    channel->sendRawMsg(msg.inner_msg_id(), msg.body().data(), (int) msg.body().size());
    return true;
}
