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

void PlayerOnlineService::onPlayerDisconnect(transport::Channel* channel)
{
    // 出表前先记下 sid/uid：出表即归还对象池，之后不能再碰 Player*
    uint32 pid = 0;
    int64 sid = 0;
    if (removePlayerByChannel(channel, &pid, &sid) == nullptr)
    {
        return;
    }
    INFO_LOG("----- on player disconnect ={} sid ={}", pid, sid);

    auto it = std::make_shared<InnerPlayerDisconnectRequest>();
    it->set_sid(sid);
    it->set_roleid(pid);
    // 帧头 playerId 同登录链路：game 侧按它定位玩家执行登出/存档
    discovery::PeerConn::sendMsg(core::SRV_TYPE_GAME, INNER_PLAYER_DISCONNECT_REQ, it, pid);
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
        // 老连接的 userData 必须同步摘除：它指向即将回池的对象，留着就是悬垂指针
        if (old->second->getChannel() != nullptr)
        {
            old->second->getChannel()->setUserData(nullptr);
        }
        Player::recycle(old->second);
        onlinePlayer.erase(old);
    }

    Player* player = Player::create(uid, sid, channel);
    // 连接上挂 Player*：掉线/心跳按连接直接取，不必遍历在线表
    channel->setUserData(player);
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

Player* PlayerOnlineService::removePlayerByChannel(transport::Channel* channel, uint32* outPid, int64* outSid)
{
    std::lock_guard<std::mutex> lk(mutex_);

    // userData 在建号时挂上、顶号时同步摘除，锁内和在线表始终一致，直接取
    Player* player = (Player*)channel->getUserData();
    if (player == nullptr)
    {
        return nullptr;
    }

    // 表里查无此人说明顶号已把旧对象清走，此连接上的指针已过期，只摘不删
    auto it = onlinePlayer.find(player->getPid());
    if (it == onlinePlayer.end() || it->second != player)
    {
        channel->setUserData(nullptr);
        return nullptr;
    }

    // 先抄走 sid/uid 再出表：recycle 后 Player* 立即可被复用
    if (outPid != nullptr) { *outPid = player->getPid(); }
    if (outSid != nullptr) { *outSid = player->getSid(); }
    onlinePlayer.erase(it);
    channel->setUserData(nullptr);
    Player::recycle(player);
    return player;
}
