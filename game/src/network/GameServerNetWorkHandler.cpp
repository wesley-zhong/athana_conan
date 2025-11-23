//
// Created by zhongweiqi on 2025/10/28.
//

#include "GameServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "../controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "core/log/XLog.h"
#include "transport/ByteUtils.h"
#include "SystemMsgHandler.h"

void GameServerNetWorkHandler::initAllMsgRegister() {
    SystemMsgHandler::registMsg();
    PlayerLoginHandler::registMsgHandler();
}

void GameServerNetWorkHandler::startLogicThread(int threadNum) {
    threadPool = new AthenaThreadPool();
    threadPool->create(2);
}

void GameServerNetWorkHandler::onNewConnect(Channel *channel) {
    INFO_LOG("on new connection ={}", channel->getAddr());
}

void GameServerNetWorkHandler::onMsg(Channel *channel, void *buff, int len) {
    uint8 *data = static_cast<uint8 *>(buff);
    data = data + 4;
    int msgId = ByteUtils::readInt32(data);
    int playerId = 999;
    data += 4;
    len -= 4;

    INFO_LOG("  === on read   channel ={} len ={} msgId ={} ", channel->getAddr(), len, msgId);
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


void GameServerNetWorkHandler::onClosed(Channel *channel) {
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}

void GameServerNetWorkHandler::onEventTrigger(Channel *channel, TriggerEventEnum reason) {
    INFO_LOG("====onEventTrigger ={}   reason ={} ", channel->getAddr(), (int)reason);
}

Thread::ThreadPool *GameServerNetWorkHandler::threadPool = nullptr;
