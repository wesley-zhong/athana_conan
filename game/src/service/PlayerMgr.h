//
// Created by zhongweiqi on 2025/10/28.
//

#ifndef ATHENA_PLAYERMGR_H
#define ATHENA_PLAYERMGR_H
#
#include <unordered_map>

#include "core/common/BaseType.h"
#include "objs/Player.h"
#include "transport/Channel.h"


class PlayerMgr {
public:
    PlayerMgr() {
    }

    ~PlayerMgr() {
    }

    Player *newPlayer(uint32_t pid, Channel *channel);

    void addPlayer(Player *player);

    void removePlayer(Player *player);

    Player *getPlayer(uint32 playerId) {
        auto it = players.find(playerId);
        if (it == players.end()) {
            return nullptr;
        }
        return it->second;
    }

private:
    std::unordered_map<uint32, Player *> players;
};


#endif //ATHENA_PLAYERMGR_H
