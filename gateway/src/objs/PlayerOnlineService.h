//
// Created by zhongweiqi on 2026/10/21.
//

#ifndef ATHENA_PLAYERONLINESERVICE_H
#define ATHENA_PLAYERONLINESERVICE_H
#include <map>
#include <mutex>
#include "Player.h"
#include  "transport/Channel.h"

// 网关侧在线玩家表：uid -> Player（对象池取出，掉线/顶号时出表归还池）。
// 消息派发在 GateActor 多逻辑线程上并发，接口内部加锁
class PlayerOnlineService
{
private:
    static std::mutex mutex_;
    static std::map<uint32, Player*> onlinePlayer;

public:
    // 登录入口（原 PlayerLoginHandler::onLoginReq 主体）：
    // 建号入表（顶号时旧对象先出表归还），再向 game 发内部登录请求
    static void onPlayerLogin(uint32 pid, int64 sid, transport::Channel* channel);

    // 掉线出口（GatewayServerNetWorkHandler::onClosed 调用）：
    // 按客户端连接反查出表归还，再向 game 发内部掉线通知触发登出/存档。
    // 不走 userData 反查——顶号回收后那里是悬垂的池对象指针
    static void onPlayerDisconnect(transport::Channel* channel);

    static Player* findPlayer(uint32 id);

    // 取池对象入表；同 uid 已在线时旧对象先归还（顶号），返回新入表的玩家
    static Player* createPlayer(uint32 uid, int64 sid, transport::Channel* channel);

    // 出表并归还对象池。返回出表的玩家指针仅作“确有此人”的凭据——对象已回池、
    // 可能被其他线程立即复用，不可再解引用；不在线返回 nullptr
    static Player* removePlayer(uint32 id);

private:
    // 按客户端连接反查出表（onPlayerDisconnect 内用）：出表前先抄走 sid/uid 到
    // 出参，供出表后发内部掉线通知；不在该连接上返回 nullptr
    static Player* removePlayerByChannel(transport::Channel* channel, uint32* outPid, int64* outSid);
};


#endif //ATHENA_PLAYERONLINESERVICE_H
