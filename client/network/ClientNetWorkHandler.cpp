//
// Created by zhongweiqi on 2025/10/28.
//

#include "ClientNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "PlayerLoginHandler.h"
#include "ProtoCommon.pb.h"
#include "transport/Channel.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"

#include "transport/EventDefs.h"
#include "ProtoMsgId.pb.h"

void ClientNetWorkHandler::initAllMsgRegister() {
    REGISTER_MSG_ID_FUN(LOGIN_RESPONSE, LoginResponse, PlayerLoginHandler::onLoginRes);
    REGISTER_MSG_ID_FUN(HEART_BEAT_RESPONSE, HeartBeatResponse, PlayerLoginHandler::onHeartBeat);
}

void ClientNetWorkHandler::startThread(int threadNum) {
    auto &sys = core::actor::ActorSystem::instance();
    for (int i = 0; i < threadNum; ++i) {
        if (core::actor::Actor *a = sys.spawn<core::actor::Actor>("client-logic-" + std::to_string(i))) {
            logicActors.push_back(a->id());
        }
    }
}

// 测试用自增登录 id；concurrentqueue 内部局部变量 id 会隐藏全局名，改名规避 C4459
int next_login_id = 100;
void ClientNetWorkHandler::onConnect(transport::Channel *channel, int status) {
    auto login = std::make_shared<LoginRequest>();
    login->set_roleid(next_login_id++);
    channel->sendMsg(LOGIN_REQUEST, login);
}

void ClientNetWorkHandler::onMsg(transport::Channel *channel, void *buff, int len) {
    INFO_LOG("  === ------------on read len={} ", len);
    uint8 *data = static_cast<uint8 *>(buff);
    data = data + 4;
    int msgId = transport::ByteUtils::readInt32(data);
    int playerId = 0;
    data += 4;
    len -= 8;

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


void ClientNetWorkHandler::onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason) {
    if (reason == transport::WRITE_IDLE) {
        auto msg = std::make_shared<HeartBeatRequest>();
        msg->set_clienttime(5555);
        channel->sendMsg(HEART_BEAT_PUSH, msg);
        // INFO_LOG("heart beat = -----------------");
        return;
    }
    // this should be closed
    if (reason == transport::READ_IDLE) {
        INFO_LOG("========== onEventTrigger ={}   reason ={} idle should closed ", channel->getAddr(), (int)reason);
    }
}


void ClientNetWorkHandler::onClosed(transport::Channel *channel) {
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}


std::vector<uint64> ClientNetWorkHandler::logicActors;
