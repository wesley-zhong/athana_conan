//
// Created by zhongweiqi on 2025/10/28.
//

#include "GameServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "../controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"
#include "SystemMsgHandler.h"

void GameServerNetWorkHandler::initAllMsgRegister() {
    SystemMsgHandler::registMsg();
    PlayerLoginHandler::registMsgHandler();
}

void GameServerNetWorkHandler::startLogicThread(int threadNum) {
    auto &sys = core::actor::ActorSystem::instance();
    for (int i = 0; i < threadNum; ++i) {
        if (core::actor::Actor *a = sys.spawn<core::actor::Actor>("game-logic-" + std::to_string(i))) {
            logicActors.push_back(a->id());
        }
    }
}

void GameServerNetWorkHandler::onNewConnect(transport::Channel *channel) {
    INFO_LOG("on new connection ={}", channel->getAddr());
}

void GameServerNetWorkHandler::onMsg(transport::Channel *channel, void *buff, int len) {
    uint8 *data = static_cast<uint8 *>(buff);
    data += 4;
    int msgId = transport::ByteUtils::readInt32(data);
    int playerId = 999;
    data += 4;
    len -= 8;

    if (msgId != -3) {
        INFO_LOG("  === on read   channel ={} msgId ={}  len ={} ", channel->getAddr(), msgId, len);
    }
    transport::MsgFunction *msg_function = transport::Dispatcher::Instance()->findMsgFuncion(msgId);
    if (msg_function == nullptr) {
        ERR_LOG(" msgId ={} not found process function", msgId);
        return;
    }

    void *msg = msg_function->parseParam(data, len);
    core::actor::ActorSystem::instance().execute(logicActors[2 % logicActors.size()],
                                           [playerId, msg_function, channel, msg]() {
                                               msg_function->invoke(playerId, channel, msg);
                                           });
}


void GameServerNetWorkHandler::onClosed(transport::Channel *channel) {
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}

void GameServerNetWorkHandler::onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason) {
    INFO_LOG("====onEventTrigger ={}   reason ={} ", channel->getAddr(), (int)reason);
}

std::vector<uint64> GameServerNetWorkHandler::logicActors;
