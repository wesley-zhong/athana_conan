//
// Created by zhongweiqi on 2026/9/23.
//

#include "RouterServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "transport/Channel.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"
#include "ProtoMsgId.pb.h"
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

//|---4 msgLen|----4 msgId|-----4 playeId |------ 4 crc| ------ body|
void RouterServerNetWorkHandler::onMsg(transport::Channel* channel, void* buff, int len)
{
    uint8* data = static_cast<uint8*>(buff);
    data = data + 4;
    int msgId = transport::ByteUtils::readInt32(data);
    len -= 8;

    transport::MsgFunction* msg_function = transport::Dispatcher::Instance()->findMsgFuncion(msgId);
    if (msg_function == nullptr)
    {
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

    int hashCode = (int)((uintptr_t)channel);
    RouterActor::execute(LOGIC, hashCode, [msg_function, channel_ptr, msg]()
    {
        msg_function->invoke(0, channel_ptr.get(), msg);
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
