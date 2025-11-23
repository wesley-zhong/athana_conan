//
// Created by zhongweiqi on 2025/10/28.
//

#include "GatewayServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "../controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "core/log/XLog.h"
#include "transport/ByteUtils.h"
#include "ProtoInner.pb.h"


void GatewayServerNetWorkHandler::initAllMsgRegister() {
}

void GatewayServerNetWorkHandler::startLogicThread(int threadNum) {
    threadPool = new AthenaThreadPool();
    threadPool->create(threadNum);
}

void GatewayServerNetWorkHandler::onConnect(Channel *channel) {
    INFO_LOG("++++++++  on new connection ={}", channel->getAddr());
}

void GatewayServerNetWorkHandler::onMsg(Channel *channel, void *buff, int len) {
    uint8 *data = static_cast<uint8 *>(buff);
    data = data + 4;
    int msgId = ByteUtils::readInt32(data);
    int playerId = 0;
    len -= 4;
    INFO_LOG("=== on read   channel ={} len ={} msgId={}", channel->getAddr(), len,msgId);

    //first check all msg_id valid
    MsgFunction *msg_function = Dispatcher::Instance()->findMsgFuncion(msgId);
    if (msg_function == nullptr) {
        proxyMsgToGame(channel, (char*)buff, len);
        ERR_LOG(" msgId ={} not found process function", msgId);
        return;
    }


    void *msg = msg_function->newParam((char *) data + 4, len);
    threadPool->execute([playerId, msg_function, channel, msg]() {
        msg_function->msgFunction(playerId, channel, msg);
    }, 2);
}


void GatewayServerNetWorkHandler::onClosed(Channel *channel) {
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}

void GatewayServerNetWorkHandler::onEventTrigger(Channel *channel, TriggerEventEnum reason) {
    if (reason == READ_IDLE) {
        INFO_LOG("========== onEventTrigger ={}    READ_IDLE  should close it  ", channel->getAddr());
        channel->close();
    }
}

void GatewayServerNetWorkHandler::proxyMsgToGame(Channel *channel,char *buff, int len) {

}


Thread::ThreadPool *GatewayServerNetWorkHandler::threadPool = nullptr;
