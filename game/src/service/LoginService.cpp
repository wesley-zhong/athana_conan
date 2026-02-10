//
// Created by zhongweiqi on 2026/2/9.
//

#include "LoginService.h"
#include "core/log/XLog.h"

void LoginService::onPlayerLogin(Channel *channel, InnerLoginRequest *req) {
    Player *existPlayer = playerMgr->getPlayer(req->roleid());
    if (existPlayer != nullptr) {
        existPlayer->setChannel(channel);
    } else {
        existPlayer = playerMgr->newPlayer(req->roleid(), channel);
        existPlayer->initModules();
        playerMgr->addPlayer(existPlayer);
    }
    auto res = std::make_shared<InnerLoginResponse>();
    res->set_roleid(req->roleid());
    res->set_sid(req->sid());
    channel->sendMsg(INNER_TO_GAME_LOGIN_RES, res);
}

void LoginService::onPlayerDisconnect(uint64 playerId, InnerPlayerDisconnectRequest *req) {
    Player *existPlayer = playerMgr->getPlayer(playerId);
    if (existPlayer == nullptr) {
        INFO_LOG("player id ={} disconnected not founded", playerId);
        return;
    }
    Channel *playerChannel = existPlayer->getChannel();
    if (playerChannel == nullptr) {
        INFO_LOG("player id = {}   disconnected", playerId);
        return;
    }
    INFO_LOG("player id = {} channel ={}  disconnected", playerId, existPlayer->getChannel()->getAddr());
}