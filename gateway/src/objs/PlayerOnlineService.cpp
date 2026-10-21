//
// Created by zhongweiqi on 2026/10/21.
//

#include "PlayerOnlineService.h"

#include "log/XLog.h"
#include "ProtoInner.pb.h"
#include "discovery/PeerConn.h"

std::mutex PlayerOnlineService::mutex_;
std::map<uint32, Player*> PlayerOnlineService::onlinePlayer;

void PlayerOnlineService::onPlayerLogin(uint32 pid, int64 sid, transport::Channel* channel)
{
    createPlayer(pid, sid, channel);
    INFO_LOG("----- on login req ={} sid ={}", pid, sid);

    auto it = std::make_shared<InnerLoginRequest>();
    it->set_sid(sid);
    it->set_roleid(pid);
    // 帧头带上 playerId：game 侧回包按它路由回发起登录的客户端
    discovery::PeerConn::sendMsg(core::SRV_TYPE_GAME, INNER_TO_GAME_LOGIN_REQ, it, pid);
}

Player* PlayerOnlineService::findPlayer(uint32 id)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = onlinePlayer.find(id);
    return it == onlinePlayer.end() ? nullptr : it->second;
}

Player* PlayerOnlineService::createPlayer(uint32 uid, int64 sid, transport::Channel* channel)
{
    // 全程持锁：并发登录同一 uid 时，出表/入表不交错，池对象不会遗留在表外
    std::lock_guard<std::mutex> lk(mutex_);

    // 顶号/重连：同 uid 旧玩家先出表归还，池对象不归还会被永久占用
    auto old = onlinePlayer.find(uid);
    if (old != onlinePlayer.end())
    {
        WARN_LOG("player ={} already online, kick old session and relogin", uid);
        Player::recycle(old->second);
        onlinePlayer.erase(old);
    }

    Player* player = Player::create(uid, sid, channel);
    onlinePlayer[uid] = player;
    return player;
}

Player* PlayerOnlineService::removePlayer(uint32 id)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = onlinePlayer.find(id);
    if (it == onlinePlayer.end())
    {
        return nullptr;
    }
    Player* player = it->second;
    onlinePlayer.erase(it);
    // 归还后可能被其他线程立即取出复用，调用方不得再碰该指针
    Player::recycle(player);
    return player;
}
