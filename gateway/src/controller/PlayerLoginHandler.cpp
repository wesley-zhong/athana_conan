#include "PlayerLoginHandler.h"

#include "log/XLog.h"
#include "ProtoCommon.pb.h"
#include "ProtoInner.pb.h"
#include "transport/Dispatcher.h"
#include "discovery/PeerConn.h"
#include "objs/PlayerOnlineService.h"
#include "utils/Snowflake.h"

void PlayerLoginHandler::onInnerLoginRes(transport::Channel* channel, InnerLoginResponse* res)
{
    Player* existPlayer = PlayerOnlineService::findPlayer(res->roleid());
    if (existPlayer == nullptr)
    {
        ERR_LOG("----- on login res  roleId={} sid ={} not found the player", res->roleid(), res->sid());
        return;
    }
    existPlayer->setValid();
    INFO_LOG("----- on login res  roleId={} sid ={} success", res->roleid(), res->sid());
}

void PlayerLoginHandler::onLoginReq(transport::Channel* channel, LoginRequest* req)
{
    int64 sid = core::Snowflake::nextId();
    // 建号入表 + 转 game 内部登录，统一收在 PlayerOnlineService
    PlayerOnlineService::onPlayerLogin((uint32)req->roleid(), sid, channel);
}

void PlayerLoginHandler::onHeartBeat(transport::Channel* channel, HeartBeatRequest* req)
{
    Player* player = (Player*)channel->getUserData();
    if (player == nullptr)
    {
        return;
    }
    auto res = std::make_shared<HeartBeatResponse>();
    res->set_servertime(core::XTime::currentTimeMillis());
    channel->sendMsg(HEART_BEAT_RESPONSE, res);
}

void PlayerLoginHandler::registMsgHandler()
{
    REGISTER_MSG_ID_FUN(INNER_TO_GAME_LOGIN_RES, InnerLoginResponse, PlayerLoginHandler::onInnerLoginRes);
    REGISTER_MSG_ID_FUN(LOGIN_REQUEST, LoginRequest, PlayerLoginHandler::onLoginReq);
    REGISTER_MSG_ID_FUN(HEART_BEAT_REQUEST, HeartBeatRequest, PlayerLoginHandler::onHeartBeat);
}
