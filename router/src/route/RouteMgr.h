//
// Created by zhongweiqi on 2026/9/23.
//

#ifndef ATHENA_ROUTEMGR_H
#define ATHENA_ROUTEMGR_H

#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "transport/Channel.h"
#include "ProtoInner.pb.h"

namespace core { struct NodeInfo; }

// game 间消息路由表：
// - 每个 player 固定映射到一个 game 节点（一致性由注册侧保证，router 只记映射）；
// - INNER_ROUTE_TRANSFER 到达后按 player_id 查表定点转发，
//   查不到时按 dest_service_id 定点、再不行对全部 game 节点扩散。
class RouteMgr {
public:
    static void registMsg();

    // player -> 目标 game 节点登记（game 侧玩家上线/迁移时调用）
    static void bindPlayer(int64 playerId, const std::string &serviceId);
    static void unbindPlayer(int64 playerId);

    static std::string findPlayerServiceId(int64 playerId);

private:
    static void onRouteTransfer(transport::Channel *channel, InnerRouteTransfer *msg);

    // 定点转发：按 service_id 找不到节点时返回 false
    static bool forwardToService(const std::string &serviceId, const InnerRouteTransfer &msg);

    static std::mutex mutex_;
    static std::unordered_map<int64, std::string> player_routes_; // player_id -> service_id
};


#endif //ATHENA_ROUTEMGR_H
