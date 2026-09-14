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
    auto &sys = actor::ActorSystem::instance();
    for (int i = 0; i < threadNum; ++i) {
        if (actor::Actor *a = sys.spawn<actor::Actor>("game-logic-" + std::to_string(i))) {
            logicActors.push_back(a->id());
        }
    }
}

void GameServerNetWorkHandler::onNewConnect(Channel *channel) {
    INFO_LOG("on new connection ={}", channel->getAddr());
}

void GameServerNetWorkHandler::onMsg(Channel *channel, void *buff, int len) {
    uint8 *data = static_cast<uint8 *>(buff);
    data += 4;
    int msgId = ByteUtils::readInt32(data);
    int playerId = 999;
    data += 4;
    len -= 8;

    if (msgId != -3) {
        INFO_LOG("  === on read   channel ={} msgId ={}  len ={} ", channel->getAddr(), msgId, len);
    }
    MsgFunction *msg_function = Dispatcher::Instance()->findMsgFuncion(msgId);
    if (msg_function == nullptr) {
        ERR_LOG(" msgId ={} not found process function", msgId);
        return;
    }

    void *msg = msg_function->parseParam(data, len);
    actor::ActorSystem::instance().execute(logicActors[2 % logicActors.size()],
                                           [playerId, msg_function, channel, msg]() {
                                               msg_function->invoke(playerId, channel, msg);
                                           });
}


void GameServerNetWorkHandler::onClosed(Channel *channel) {
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}

void GameServerNetWorkHandler::onEventTrigger(Channel *channel, TriggerEventEnum reason) {
    INFO_LOG("====onEventTrigger ={}   reason ={} ", channel->getAddr(), (int)reason);
}

std::vector<uint64> GameServerNetWorkHandler::logicActors;
