//
// Created by zhongweiqi on 2025/10/31.
//

#include "SystemMsgHandler.h"
#include "core/log/XLog.h"
#include "ProtoInner.pb.h"
#include "transport/Dispatcher.h"

void SystemMsgHandler::onShakHandReq(Channel *channel, InnerServerHandShakeReq *req) {
    INFO_LOG(" receive shake req hand msg ={} innherHeaderId = {}", channel->getAddr(), req->service_id());
    auto res = std::make_shared<InnerServerHandShakeRes>();
    res->set_service_id("9999ttt");
    channel->sendMsg(INNER_SERVER_HAND_SHAKE_RES, res);
}

void SystemMsgHandler::onShakHandResponse(Channel *channel, InnerServerHandShakeRes *res) {
    INFO_LOG(" receive shake  reshand msg ={} innherHeaderId = {}", channel->getAddr(), res->service_id());
}


void SystemMsgHandler::onHeartBeatReq(Channel *channel, InnerHeartBeatRequest *req) {
    auto res = std::make_shared<InnerHeartBeatRequest>();
    INFO_LOG("inner heatbeat time ={}", req->time());
    channel->sendMsg(INNER_HEART_BEAT_RES, res);
}

void SystemMsgHandler::onHeartBeatRes(Channel *channel, InnerHeartBeatResponse *res) {
}

void SystemMsgHandler::registMsg() {
    REGISTER_MSG_ID_FUN(INNER_SERVER_HAND_SHAKE_REQ, InnerServerHandShakeReq, onShakHandReq);
    REGISTER_MSG_ID_FUN(INNER_SERVER_HAND_SHAKE_RES, InnerServerHandShakeRes, onShakHandResponse);
    REGISTER_MSG_ID_FUN(INNER_HEART_BEAT_REQ, InnerHeartBeatRequest, onHeartBeatReq);
    REGISTER_MSG_ID_FUN(INNER_HEART_BEAT_RES, InnerHeartBeatResponse, onHeartBeatRes);
}
