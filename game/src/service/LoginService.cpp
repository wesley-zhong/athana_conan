//
// Created by zhongweiqi on 2026/2/9.
//

#include "LoginService.h"

void LoginService::onPlayerLogin(Channel *channel, InnerLoginRequest *req) {
    auto res = std::make_shared<InnerLoginResponse>();
    channel->sendMsg(INNER_TO_GAME_LOGIN_RES, res);
}

void LoginService::onPlayerDisconnect(uint64 playerId, InnerPlayerDisconnectRequest *req) {

}