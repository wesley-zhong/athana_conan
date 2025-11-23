//
// Created by zhongweiqi on 2025/10/28.
//

#include "service/PlayerMgr.h"

#include "core/common/ObjectPool.hpp"

void PlayerMgr::addPlayer(Player *player) {
    players.insert({player->getPid(), player});
}

void PlayerMgr::removePlayer(Player *player) {
    players.erase(player->getPid());
}


Player *PlayerMgr::newPlayer(uint32 playerId, Channel *channel) {
    return ObjPool::acquirePtr<Player>(playerId, channel);
}
