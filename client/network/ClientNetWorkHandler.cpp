//
// Created by zhongweiqi on 2025/10/28.
//

#include "ClientNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "PlayerLoginHandler.h"
#include "ProtoCommon.pb.h"
#include "transport/Channel.h"
#include "transport/Decoder.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"

#include "transport/EventDefs.h"

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
    // 客户端链路帧头无 playerId 字段，归属信息走 body 的 proto roleid
    channel->sendMsg(LOGIN_REQUEST, login, login->roleid());
}

void ClientNetWorkHandler::onMsg(transport::Channel *channel, void *buff, int len) {
    // 拆帧头 + 查注册表 + 反序列化 + 取 channel shared_ptr，集中在 Decoder
    transport::DecodedMsg decoded;
    transport::Decoder::parseHeader(channel, buff, len, &decoded);
    switch (transport::Decoder::decodeMsg(channel, &decoded)) {
        case transport::DecodeResult::NOT_REGISTERED:
            ERR_LOG(" msgId ={} not found process function", decoded.msgId);
            return;
        case transport::DecodeResult::PARSE_FAILED:
        case transport::DecodeResult::BAD_HEADER:
        case transport::DecodeResult::CHANNEL_GONE:
            return;
        case transport::DecodeResult::OK:
            break;
    }
    INFO_LOG("  === ------------on read len={} remote addr ={}  msgId= {}", len, channel->getAddr(), decoded.msgId);
    core::actor::ActorSystem::instance().execute(logicActors[2 % logicActors.size()],
                                           [playerId = decoded.playerId,
                                            msg_function = decoded.msg_function,
                                            channel_ptr = decoded.channel_ptr,
                                            msg = decoded.msg]() {
                                               msg_function->invoke(playerId, channel_ptr.get(), msg);
                                           });
}


void ClientNetWorkHandler::onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason) {
    if (reason == transport::WRITE_IDLE) {
        auto msg = std::make_shared<HeartBeatRequest>();
        msg->set_clienttime(5555);
        channel->sendMsg(HEART_BEAT_REQUEST, msg);
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
