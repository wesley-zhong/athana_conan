//
// Created by zhongweiqi on 2025/10/28.
//

#include "GateClientNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "core/log/XLog.h"
#include "transport/ByteUtils.h"

#include "ProtoInner.pb.h"
#include "SystemMsgHandler.h"
#include "discovery/AthenaDiscovery.h"

void GateClientNetWorkHandler::initAllMsgRegister()
{
    SystemMsgHandler::registMsg();
    PlayerLoginHandler::registMsgHandler();
}

void GateClientNetWorkHandler::startLogicThread(int threadNum)
{
    auto &sys = core::actor::ActorSystem::instance();
    for (int i = 0; i < threadNum; ++i) {
        if (core::actor::Actor *a = sys.spawn<core::actor::Actor>("gate-logic-" + std::to_string(i))) {
            logicActors.push_back(a->id());
        }
    }
}

void GateClientNetWorkHandler::onNewConnect(transport::Channel* channel, int status)
{
    INFO_LOG("on new connection ={}", channel->getAddr());
    auto req = std::make_shared<InnerServerHandShakeReq>();
    std::shared_ptr<core::NodeInfo> shNodeInfo = discovery::AthenaDiscovery::Instance()->getMySelf();
    req->set_service_id(shNodeInfo->service_id);
    req->set_service_name(shNodeInfo->service_name);
    req->set_server_type(shNodeInfo->type);
    channel->sendMsg(INNER_SERVER_HAND_SHAKE_REQ, req);
}

void GateClientNetWorkHandler::onMsg(transport::Channel* channel, void* buff, int len)
{
    uint8* data = static_cast<uint8*>(buff);
    data += 4;
    int msgId = transport::ByteUtils::readInt32(data);
    int playerId = 999;
    data += 4;
    len -= 8;
    // INFO_LOG("=== on read   channel ={}  msgId={}  len ={}", channel->getAddr(), msgId, len);
    transport::MsgFunction* msg_function = transport::Dispatcher::Instance()->findMsgFuncion(msgId);
    if (msg_function == nullptr)
    {
        ERR_LOG("msgId ={} not found process function", msgId);
        return;
    }

    void* msg = msg_function->parseParam(data, len);
    core::actor::ActorSystem::instance().execute(logicActors[2 % logicActors.size()],
                                           [playerId, msg_function, channel, msg]()
                                           {
                                               msg_function->invoke(playerId, channel, msg);
                                           });
}

void GateClientNetWorkHandler::onEventTrigger(transport::Channel* channel, transport::TriggerEventEnum reason)
{
    if (reason == transport::WRITE_IDLE)
    {
        auto msg = std::make_shared<InnerHeartBeatRequest>();
        msg->set_time(8888);
        channel->sendMsg(INNER_HEART_BEAT_REQ, msg);
        // INFO_LOG("heart beat = -----------------");
        return;
    }
    //    // this should be closed
    if (reason == transport::READ_IDLE)
    {
        INFO_LOG("========== onEventTrigger ={}   reason ={} idle should closed ", channel->getAddr(), (int)reason);
    }
}


void GateClientNetWorkHandler::onClosed(transport::Channel* channel)
{
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}


std::vector<uint64> GateClientNetWorkHandler::logicActors;
