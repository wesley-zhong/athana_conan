//
// Created by zhongweiqi on 2026/2/9.
//

#include "LoginService.h"
#include "log/XLog.h"

void LoginService::onPlayerLogin(transport::Channel* channel, InnerLoginRequest* req)
{
    // proto roleId 为 int64，Player 体系用 uint32，此处收窄是有意为之
    uint32 playerId = (uint32)req->roleid();
    Player* existPlayer = playerMgr->getPlayer(playerId);
    if (existPlayer != nullptr)
    {
        existPlayer->setChannel(channel);
    }
    else
    {
        existPlayer = playerMgr->newPlayer(playerId, req->sid(), channel);
        existPlayer->initModules();
        //first load data from db
        existPlayer->loadDataFormDB();
        // second call on login logic process
    }
    existPlayer->onLogin();
    playerMgr->addPlayer(existPlayer);
    //return client req
    auto res = std::make_shared<InnerLoginResponse>();
    res->set_roleid(req->roleid());
    res->set_sid(req->sid());
    channel->sendMsg(INNER_TO_GAME_LOGIN_RES, res);

    // only for test
    //existPlayer->saveDataToDB();
}

void LoginService::onPlayerDisconnect(uint32 playerId, InnerPlayerDisconnectRequest* req)
{
    Player* existPlayer = playerMgr->getPlayer(playerId);
    if (existPlayer == nullptr)
    {
        INFO_LOG("player id ={} disconnected not founded", playerId);
        return;
    }
    transport::Channel* playerChannel = existPlayer->getChannel();
    if (playerChannel == nullptr)
    {
        INFO_LOG("player id = {}   disconnected", playerId);
        return;
    }
    INFO_LOG("player id = {} channel ={}  disconnected", playerId, existPlayer->getChannel()->getAddr());

    existPlayer->onLogout();

    existPlayer->saveDataToDB();
}
