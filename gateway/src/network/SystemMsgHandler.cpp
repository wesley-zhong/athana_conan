//
// Created by zhongweiqi on 2025/10/31.
//

#include "SystemMsgHandler.h"
#include "core/log/XLog.h"
#include "ProtoInner.pb.h"
#include "transport/Dispatcher.h"
#include "discovery/PeerConn.h"
#include "discovery/AthenaDiscovery.h"

void SystemMsgHandler::onShakHandReq(Channel *channel, InnerServerHandShakeReq *req) {
    INFO_LOG("receive shake req hand msg ={} innherHeaderId = {}", channel->getAddr(), req->service_id());
    auto shNodeInfo = std::make_unique<NodeInfo>();
    shNodeInfo->type = req->server_type();
    shNodeInfo->service_name = req->service_name();
    shNodeInfo->service_id = req->service_id();

    PeerConn::saveNode(std::move(shNodeInfo));
    PeerConn::saveNodeChannel(req->service_id(), channel);

    auto res = std::make_shared<InnerServerHandShakeRes>();
    std::shared_ptr<NodeInfo> mySelf = AthenaDiscovery::Instance()->getMySelf();
    res->set_service_id(mySelf->service_id);
    res->set_service_name(mySelf->service_name);
    channel->sendMsg(INNER_SERVER_HAND_SHAKE_RES, res);
}

void SystemMsgHandler::onShakHandResponse(Channel *channel, InnerServerHandShakeRes *res) {
    INFO_LOG("receive shakehand  res msg ={} innherHeaderId = {}", channel->getAddr(), res->service_id());
    PeerConn::saveNodeChannel(res->service_id(), channel);
}


void SystemMsgHandler::onInnerHeartBeatReq(Channel *channel, InnerHeartBeatRequest *req) {
    auto res = std::make_shared<InnerHeartBeatRequest>();
    channel->sendMsg(INNER_HEART_BEAT_RES, res);
}

void SystemMsgHandler::onInnerHeartBeatRes(Channel *channel, InnerHeartBeatResponse *res) {
    //  INFO_LOG("#### receive   on Inner HeartBeatRes msg ={} time = {}", channel->getAddr(), res->time());
}

void SystemMsgHandler::registMsg() {
    REGISTER_MSG_ID_FUN(INNER_SERVER_HAND_SHAKE_REQ, InnerServerHandShakeReq, onShakHandReq);
    REGISTER_MSG_ID_FUN(INNER_SERVER_HAND_SHAKE_RES, InnerServerHandShakeRes, onShakHandResponse);
    REGISTER_MSG_ID_FUN(INNER_HEART_BEAT_REQ, InnerHeartBeatRequest, onInnerHeartBeatReq);
    REGISTER_MSG_ID_FUN(INNER_HEART_BEAT_RES, InnerHeartBeatResponse, onInnerHeartBeatRes);
}
