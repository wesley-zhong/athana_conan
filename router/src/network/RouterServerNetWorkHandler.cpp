//
// Created by zhongweiqi on 2026/9/23.
//

#include "RouterServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "transport/Channel.h"
#include "transport/Decoder.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"
#include "ProtoInner.pb.h"
#include "SystemMsgHandler.h"
#include "route/RouteMgr.h"
#include "discovery/PeerConn.h"
#include "../router_actor/RouterActor.h"


void RouterServerNetWorkHandler::initAllMsgRegister()
{
    SystemMsgHandler::registMsg();
    RouteMgr::registMsg();
}

void RouterServerNetWorkHandler::startLogicThread(int ioThreadNum, int logicThreadNum)
{
    RouterActor::initActors(IO, ioThreadNum);
    RouterActor::initActors(LOGIC, logicThreadNum);
}

void RouterServerNetWorkHandler::onConnect(transport::Channel* channel)
{
    INFO_LOG("++++++++  on new connection ={}", channel->getAddr());
}

//|---4 msgLen|----4 msgId|-----4 playerId|------4 magicNum|------body|
void RouterServerNetWorkHandler::onMsg(transport::Channel* channel, void* buff, int len)
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

    int hashCode = (int)((uintptr_t)channel);
    RouterActor::execute(LOGIC, hashCode, [playerId = decoded.playerId,
                                           msg_function = decoded.msg_function,
                                           channel_ptr = decoded.channel_ptr,
                                           msg = decoded.msg]()
    {
        msg_function->invoke(playerId, channel_ptr.get(), msg);
    });
}


void RouterServerNetWorkHandler::onClosed(transport::Channel* channel)
{
    INFO_LOG("connection ={}  closed ", channel->getAddr());
    // 与 game 的连接断开时摘除登记，避免后续消息发往已关闭的连接
    discovery::PeerConn::removeNodeChannel(channel);
}

void RouterServerNetWorkHandler::onEventTrigger(transport::Channel* channel, transport::TriggerEventEnum reason)
{
    if (reason == transport::READ_IDLE)
    {
        INFO_LOG("========== onEventTrigger ={}    READ_IDLE  should close it  ", channel->getAddr());
        channel->close();
    }
}
