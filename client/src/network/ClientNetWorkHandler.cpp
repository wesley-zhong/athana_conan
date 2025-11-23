//
// Created by zhongweiqi on 2025/10/28.
//

#include "ClientNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "PlayerLoginHandler.h"
#include "ProtoCommon.pb.h"
#include "transport/Channel.h"
#include "core/log/XLog.h"
#include "transport/ByteUtils.h"

#include "ProtoInner.pb.h"
#include "transport/EventDefs.h"

void ClientNetWorkHandler::initAllMsgRegister() {
    REGISTER_MSG_ID_FUN(LOGIN_RESPONSE, LoginResponse, PlayerLoginHandler::onLoginRes);
    REGISTER_MSG_ID_FUN(HEART_BEAT_RESPONSE, HeartBeatResponse, PlayerLoginHandler::onHeartBeat);
}

void ClientNetWorkHandler::startThread(int threadNum) {
    threadPool = new AthenaThreadPool();
    threadPool->create(threadNum);
}

void ClientNetWorkHandler::onConnect(Channel *channel, int status) {
    auto login = std::make_shared<LoginRequest>();
    login->set_roleid(123);
    channel->sendMsg(LOGIN_REQUEST, login);
}

void ClientNetWorkHandler::onMsg(Channel *channel, void *buff, int len) {
    INFO_LOG("  === ------------on read len={} ", len);
    uint8 *data = static_cast<uint8 *>(buff);
    data = data + 4;
    int msgId = ByteUtils::readInt32(data);
    int playerId = 0;
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


void ClientNetWorkHandler::onEventTrigger(Channel *channel, TriggerEventEnum reason) {
    if (reason == WRITE_IDLE) {
        auto msg = std::make_shared<HeartBeatRequest>();
        msg->set_clienttime(5555);
        channel->sendMsg(HEART_BEAT_REQUEST, msg);
        // INFO_LOG("heart beat = -----------------");
        return;
    }
    // this should be closed
    if (reason == READ_IDLE) {
        INFO_LOG("========== onEventTrigger ={}   reason ={} idle should closed ", channel->getAddr(), (int)reason);
    }
}


void ClientNetWorkHandler::onClosed(Channel *channel) {
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}


Thread::ThreadPool *ClientNetWorkHandler::threadPool = nullptr;
