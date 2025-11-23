#include "PlayerLoginHandler.h"

#include "core/common/BaseType.h"
#include "core/log/XLog.h"

#include "ProtoInner.pb.h"


void PlayerLoginHandler::onLoginRes(Channel *channel, LoginResponse *res) {
    INFO_LOG("----- on login res  roleId  ={}", res->roleid());
}

void PlayerLoginHandler::onHeartBeat(Channel *channel, HeartBeatResponse *res) {
    INFO_LOG("----- on onHeartBeat  server time ={}", res->servertime());
}
