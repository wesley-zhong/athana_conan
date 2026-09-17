//
// Created by zhongweiqi on 2026/2/9.
//

#ifndef ATHENA_LOGINSERVICE_H
#define ATHENA_LOGINSERVICE_H

#include <memory>
#include "ProtoInner.pb.h"
#include "transport/Channel.h"
#include "PlayerMgr.h"

class LoginService
{
public:
    static void onPlayerLogin(transport::Channel* channel, InnerLoginRequest* req);

    static void onPlayerDisconnect(uint64 playerId, InnerPlayerDisconnectRequest* req);


    static Player* findPlayer(uint64 playerId)
    {
        return playerMgr->getPlayer(playerId);
    }

private:
    inline static std::unique_ptr<PlayerMgr> playerMgr = std::make_unique<PlayerMgr>();
};


#endif //ATHENA_LOGINSERVICE_H
