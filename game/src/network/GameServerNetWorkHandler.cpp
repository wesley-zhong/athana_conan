//
// Created by zhongweiqi on 2025/10/28.
//

#include "GameServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "../controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"
#include "SystemMsgHandler.h"
#include "discovery/PeerConn.h"

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
    if (msg == nullptr) {
        ERR_LOG("parse msg failed, msgId ={}", msgId);
        return;
    }
    // actor 任务异步执行，channel 必须用 shared_ptr 持有，防止连接关闭后被释放
    std::shared_ptr<transport::Channel> channel_ptr = channel->event_loop()->channelPtr(channel);
    if (channel_ptr == nullptr) {
        return;
    }
    core::actor::ActorSystem::instance().execute(logicActors[2 % logicActors.size()],
                                           [playerId, msg_function, channel_ptr, msg]() {
                                               msg_function->invoke(playerId, channel_ptr.get(), msg);
                                           });
}


void GameServerNetWorkHandler::onClosed(transport::Channel *channel) {
    INFO_LOG("connection ={}  closed ", channel->getAddr());
    // 从服务发现登记里摘除，避免后续消息发往已关闭的连接
    discovery::PeerConn::removeNodeChannel(channel);
}

void GameServerNetWorkHandler::onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason) {
    INFO_LOG("====onEventTrigger ={}   reason ={} ", channel->getAddr(), (int)reason);
}

std::vector<uint64> GameServerNetWorkHandler::logicActors;
