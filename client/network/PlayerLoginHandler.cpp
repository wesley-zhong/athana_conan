#include "PlayerLoginHandler.h"

#include "common/BaseType.h"
#include "log/XLog.h"

#include "ProtoInner.pb.h"


void PlayerLoginHandler::onLoginRes(transport::Channel *channel, LoginResponse *res) {
    INFO_LOG("----- on login res  roleId  ={}", res->roleid());
}

void PlayerLoginHandler::onHeartBeat(transport::Channel *channel, HeartBeatResponse *res) {
    INFO_LOG("----- on onHeartBeat  server time ={}", res->servertime());
}
