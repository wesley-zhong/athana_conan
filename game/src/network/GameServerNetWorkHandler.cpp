//
// Created by zhongweiqi on 2025/10/28.
//

#include "GameServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "../controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "transport/Decoder.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"
#include "SystemMsgHandler.h"
#include "discovery/PeerConn.h"
#include "game_actor/GameActor.h"

void GameServerNetWorkHandler::initAllMsgRegister()
{
    SystemMsgHandler::registMsg();
    PlayerLoginHandler::registMsgHandler();
}

void GameServerNetWorkHandler::startLogicThread(int ioThread, int logicThread, int dbThread)
{
    GameActor::initActors(IO, ioThread);
    GameActor::initActors(LOGIC, logicThread);
    GameActor::initActors(DB, dbThread);
}

void GameServerNetWorkHandler::onNewConnect(transport::Channel* channel)
{
    INFO_LOG("on new connection ={}", channel->getAddr());
}

void GameServerNetWorkHandler::onMsg(transport::Channel* channel, void* buff, int len)
{
    // 拆帧头 + 查注册表 + 反序列化 + 取 channel shared_ptr，集中在 Decoder
    transport::DecodedMsg decoded;
    transport::Decoder::parseHeader(channel, buff, len, &decoded);
    switch (transport::Decoder::decodeMsg(channel, &decoded))
    {
        case transport::DecodeResult::NOT_REGISTERED:
            ERR_LOG(" msgId ={} not found process function", decoded.msgId);
            return;
        case transport::DecodeResult::PARSE_FAILED:
        case transport::DecodeResult::BAD_HEADER:
        case transport::DecodeResult::CHANNEL_GONE:
            return;
        case transport::DecodeResult::OK:
            break;
    }
    int msgId = decoded.msgId;
    if (msgId != -3)
    {
        INFO_LOG("  === on read   channel ={} msgId ={}  len ={} ", channel->getAddr(), msgId, decoded.bodyLen);
    }

    // core::actor::ActorSystem::instance().execute(logicActors[2 % logicActors.size()],
    //                                              [decoded]()
    //                                              {
    //                                                  decoded.msg_function->invoke(decoded.playerId, decoded.channel_ptr.get(), decoded.msg);
    //                                              });

    // 用 channel 地址做 hash：同一连接（同一玩家会话）的消息串行到同一 actor，
    // 避免乱序；TODO(session): 换成真正的 playerId 做 actor 分片
    int hashCode = (int)((uintptr_t)channel);
    GameActor::execute(LOGIC, hashCode, [playerId = decoded.playerId,
                                         msg_function = decoded.msg_function,
                                         channel_ptr = decoded.channel_ptr,
                                         msg = decoded.msg]()
    {
        msg_function->invoke(playerId, channel_ptr.get(), msg);
    });
}


void GameServerNetWorkHandler::onClosed(transport::Channel* channel)
{
    INFO_LOG("connection ={}  closed ", channel->getAddr());
    // 从服务发现登记里摘除，避免后续消息发往已关闭的连接
    discovery::PeerConn::removeNodeChannel(channel);
}

void GameServerNetWorkHandler::onEventTrigger(transport::Channel* channel, transport::TriggerEventEnum reason)
{
    INFO_LOG("====onEventTrigger ={}   reason ={} ", channel->getAddr(), (int)reason);
}
