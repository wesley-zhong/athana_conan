#include "PlayerLoginHandler.h"

#include "core/log/XLog.h"
#include "ProtoCommon.pb.h"
#include "ProtoInner.pb.h"
#include "transport/Dispatcher.h"
#include "discovery/PeerConn.h"

void PlayerLoginHandler::onInnerLoginRes(Channel *channel, InnerLoginResponse *res) {
    INFO_LOG("----- on login res ={}", res->roleid(), res->sid());
}

void PlayerLoginHandler::onLoginReq(Channel *channel, LoginRequest *req) {
    INFO_LOG("----- on login req ={}", req->roleid());
    auto it = new InnerLoginRequest(); // this may be use obj pool
    it->set_roleid(req->roleid());
    it->set_sid(1111);
    PeerConn::sendMsg(SRV_TYPE_GAME, INNER_TO_GAME_LOGIN_REQ, it);
}

void PlayerLoginHandler::onHeartBeat(Channel *channel, HeartBeatRequest *req) {
    auto res = std::make_shared<HeartBeatResponse>();
    res->set_servertime(7567);
    channel->sendMsg(HEART_BEAT_RESPONSE, res);
}

void PlayerLoginHandler::registMsgHandler() {
    REGISTER_MSG_ID_FUN(INNER_TO_GAME_LOGIN_RES, InnerLoginResponse, PlayerLoginHandler::onInnerLoginRes);
    REGISTER_MSG_ID_FUN(LOGIN_REQUEST, LoginRequest, PlayerLoginHandler::onLoginReq);
    REGISTER_MSG_ID_FUN(HEART_BEAT_REQUEST, HeartBeatRequest, PlayerLoginHandler::onHeartBeat);
}
