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

void GateClientNetWorkHandler::initAllMsgRegister() {
    SystemMsgHandler::registMsg();
    PlayerLoginHandler::registMsgHandler();
}

void GateClientNetWorkHandler::startLogicThread(int threadNum) {
    threadPool = new AthenaThreadPool();
    threadPool->create(threadNum);
}

void GateClientNetWorkHandler::onNewConnect(Channel *channel, int status) {
    INFO_LOG("on new connection ={}", channel->getAddr());
    auto req = std::make_shared<InnerServerHandShakeReq>();
    req->set_service_id("JJJJJJJJJ");
    channel->sendMsg(INNER_SERVER_HAND_SHAKE_REQ, req);
}

void GateClientNetWorkHandler::onMsg(Channel *channel, void *buff, int len) {
    //INFO_LOG("  === on read   channel ={} len ={}", channel->getAddr(), len);
    uint8 *data = static_cast<uint8 *>(buff);
    data = data + 4;
    int msgId = ByteUtils::readInt32(data);
    int playerId = 999;
    data += 4;
    len -= 4;

    MsgFunction *msg_function = Dispatcher::Instance()->findMsgFuncion(msgId);
    if (msg_function == nullptr) {
        ERR_LOG(" msgId ={} not found process function", msgId);
        return;
    }

    void *msg = msg_function->newParam(data, len);
    threadPool->execute([playerId, msg_function, channel, msg]() {
        msg_function->msgFunction(playerId, channel, msg);
    }, 2);
}

void GateClientNetWorkHandler::onEventTrigger(Channel *channel, TriggerEventEnum reason) {
    if (reason == WRITE_IDLE) {
        auto msg = std::make_shared<InnerHeartBeatRequest>();
        msg->set_time(8888);
        channel->sendMsg(INNER_HEART_BEAT_REQ, msg);
       // INFO_LOG("heart beat = -----------------");
        return;
    }
    // this should be closed
    if (reason == READ_IDLE) {
        INFO_LOG("========== onEventTrigger ={}   reason ={} idle should closed ", channel->getAddr(), (int)reason);
    }
}


void GateClientNetWorkHandler::onClosed(Channel *channel) {
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}


Thread::ThreadPool *GateClientNetWorkHandler::threadPool = nullptr;
