//
// Created by zhongweiqi on 2025/10/28.
//

#include "GameClientNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "transport/Decoder.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"

#include "ProtoInner.pb.h"
#include "SystemMsgHandler.h"
#include "discovery/AthenaDiscovery.h"
#include "discovery/PeerConn.h"
#include "game/src/game_actor/GameActor.h"
#include "game_actor/GameActor.h"
void GameClientNetWorkHandler::initAllMsgRegister()
{
    SystemMsgHandler::registMsg();
    PlayerLoginHandler::registMsgHandler();
}

void GameClientNetWorkHandler::onNewConnect(transport::Channel* channel, int status)
{
    INFO_LOG("on new connection ={}", channel->getAddr());
    auto req = std::make_shared<InnerServerHandShakeReq>();
    std::shared_ptr<core::NodeInfo> shNodeInfo = discovery::AthenaDiscovery::Instance()->getMySelf();
    req->set_service_id(shNodeInfo->service_id);
    req->set_service_name(shNodeInfo->service_name);
    req->set_server_type(shNodeInfo->type);
    channel->sendMsg(INNER_SERVER_HAND_SHAKE_REQ, req);
}

void GameClientNetWorkHandler::onMsg(transport::Channel* channel, void* buff, int len)
{
    // 拆帧头 + 查注册表 + 反序列化 + 取 channel shared_ptr，集中在 Decoder
    transport::DecodedMsg decoded;
    transport::Decoder::parseHeader(channel, buff, len, &decoded);
    switch (transport::Decoder::decodeMsg(channel, &decoded))
    {
        case transport::DecodeResult::NOT_REGISTERED:
            ERR_LOG("msgId ={} not found process function", decoded.msgId);
            return;
        case transport::DecodeResult::PARSE_FAILED:
        case transport::DecodeResult::BAD_HEADER:
        case transport::DecodeResult::CHANNEL_GONE:
            return;
        case transport::DecodeResult::OK:
            break;
    }
    // INFO_LOG("=== on read   channel ={}  msgId={}  len ={}", channel->getAddr(), decoded.msgId, decoded.bodyLen);

    int hashCode = (int)((uintptr_t)channel);
    GameActor::execute(LOGIC, hashCode, [playerId = decoded.playerId,
                                         msg_function = decoded.msg_function,
                                         channel_ptr = decoded.channel_ptr,
                                         msg = decoded.msg]()
                                           {
                                               msg_function->invoke(playerId, channel_ptr.get(), msg);
                                           });
}

void GameClientNetWorkHandler::onEventTrigger(transport::Channel* channel, transport::TriggerEventEnum reason)
{
    if (reason == transport::WRITE_IDLE)
    {
        auto msg = std::make_shared<InnerHeartBeatRequest>();
        msg->set_time(8888);
        channel->sendMsg(INNER_HEART_BEAT_REQ, msg);
      //  INFO_LOG("heart beat = -----------------{}", channel->getAddr());
        return;
    }
    //    // this should be closed
    if (reason == transport::READ_IDLE)
    {
        INFO_LOG("========== onEventTrigger ={}   reason ={} idle should closed ", channel->getAddr(), (int)reason);
    }
}


void GameClientNetWorkHandler::onClosed(transport::Channel* channel)
{
    INFO_LOG("connection ={}  closed ", channel->getAddr());
    // 与 game 的连接断开时摘除登记，避免后续消息发往已关闭的连接
    discovery::PeerConn::removeNodeChannel(channel);
}
