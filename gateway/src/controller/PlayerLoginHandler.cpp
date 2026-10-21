#include "PlayerLoginHandler.h"

#include "ProtoCommon.pb.h"
#include "ProtoInner.pb.h"
#include "ProtoTask.pb.h"
#include "transport/Dispatcher.h"
#include "discovery/PeerConn.h"
#include "objs/PlayerOnlineService.h"
#include "utils/Snowflake.h"
#include "common/XTime.h"

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

    // 此处 channel 是 gateway<->game 的内部链路，回客户端要走玩家自己的连接
    auto clientRes = std::make_shared<LoginResponse>();
    clientRes->set_errorcode(0);
    clientRes->set_servertime(core::XTime::currentTimeMillis());
    clientRes->set_roleid(res->roleid());
    existPlayer->getChannel()->sendMsg(LOGIN_RESPONSE, clientRes);
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
