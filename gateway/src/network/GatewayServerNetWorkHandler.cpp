//
// Created by zhongweiqi on 2025/10/28.
//

#include "GatewayServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "../controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"
#include "ProtoMsgId.pb.h"
#include "gate_actor/GateActor.h"


void GatewayServerNetWorkHandler::initAllMsgRegister()
{
}

void GatewayServerNetWorkHandler::startLogicThread(int ioThreadNum, int logicThreadNum)
{
    GateActor::initActors(IO, ioThreadNum);
    GateActor::initActors(LOGIC, logicThreadNum);
}

void GatewayServerNetWorkHandler::onConnect(transport::Channel* channel)
{
    INFO_LOG("++++++++  on new connection ={}", channel->getAddr());
}

//|---4 msgLen|----4 msgId|-----4 playeId |------ 4 crc| ------ body|
void GatewayServerNetWorkHandler::onMsg(transport::Channel* channel, void* buff, int len)
{
    uint8* data = static_cast<uint8*>(buff);
    data = data + 4;
    int msgId = transport::ByteUtils::readInt32(data);
    int playerId = 0;
    len -= 8;

    if (msgId == HEART_BEAT_PUSH)
    {
        auto res = std::make_shared<HeartBeatResponse>();
        res->set_servertime(8888);
        res->set_clienttime(7777);
        channel->sendMsg(HEART_BEAT_RESPONSE, res);
        return;
    }
    INFO_LOG("=== on read   channel ={} len ={} msgId={}", channel->getAddr(), len, msgId);

    //first check all msg_id valid
    transport::MsgFunction* msg_function = transport::Dispatcher::Instance()->findMsgFuncion(msgId);
    if (msg_function == nullptr)
    {
        proxyMsgToGame(channel, (char*)buff, len);
        ERR_LOG(" msgId ={} not found process function", msgId);
        return;
    }

    void* msg = msg_function->parseParam((char*)data + 4, len);
    if (msg == nullptr)
    {
        ERR_LOG("parse msg failed, msgId ={}", msgId);
        return;
    }
    // actor 任务异步执行，channel 必须用 shared_ptr 持有，防止连接关闭后被释放
    std::shared_ptr<transport::Channel> channel_ptr = channel->event_loop()->channelPtr(channel);
    if (channel_ptr == nullptr)
    {
        return;
    }
    // core::actor::ActorSystem::instance().execute(logicActors[2 % logicActors.size()],
    //                                              [playerId, msg_function, channel_ptr, msg]()
    //                                              {
    //                                                  msg_function->invoke(playerId, channel_ptr.get(), msg);
    //                                              });

    int msgThreadHashCode = 1;
    GateActor::execute(LOGIC, msgThreadHashCode, [playerId, msg_function, channel_ptr, msg]()
    {
        msg_function->invoke(playerId, channel_ptr.get(), msg);
    });
}


void GatewayServerNetWorkHandler::onClosed(transport::Channel* channel)
{
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}

void GatewayServerNetWorkHandler::onEventTrigger(transport::Channel* channel, transport::TriggerEventEnum reason)
{
    if (reason == transport::READ_IDLE)
    {
        INFO_LOG("========== onEventTrigger ={}    READ_IDLE  should close it  ", channel->getAddr());
        channel->close();
    }
}

void GatewayServerNetWorkHandler::proxyMsgToGame(transport::Channel* channel, char* buff, int len)
{
}
