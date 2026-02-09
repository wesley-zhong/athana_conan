#include "PlayerLoginHandler.h"
#include "core/log/XLog.h"
#include "service/LoginService.h"
#include "ProtoInner.pb.h"
#include "transport/Dispatcher.h"

void PlayerLoginHandler::registMsgHandler() {
    REGISTER_MSG_ID_FUN(INNER_TO_GAME_LOGIN_REQ, InnerLoginRequest, PlayerLoginHandler::onInnerLogin);
    REGISTER_MSG_ID_FUN(INNER_PLAYER_DISCONNECT_REQ, InnerPlayerDisconnectRequest,
                        PlayerLoginHandler::onPlayerDisconnected);
}


void PlayerLoginHandler::onInnerLogin(Channel *channel, InnerLoginRequest *request) {
    INFO_LOG(" ON INNER LOGIN sid = {} roleId ={} channel ={}", request->sid(), request->roleid(),
             channel->getAddr());

    LoginService::onPlayerLogin(channel, request);
}


void PlayerLoginHandler::onPlayerDisconnected(uint64 playerId, InnerPlayerDisconnectRequest *req) {
    LoginService::onPlayerDisconnect(playerId, req);
}
