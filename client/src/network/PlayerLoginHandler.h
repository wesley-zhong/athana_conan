#ifndef MSGHANDLER_H_
#define MSGHANDLER_H_

#include "core/common/BaseType.h"
#include "ProtoCommon.pb.h"

#include "ProtoTask.pb.h"
#include "transport/Channel.h"


class PlayerLoginHandler {
public:
    static void onLoginRes(Channel *channel, LoginResponse *res);

    static void onHeartBeat(Channel *channel, HeartBeatResponse *res);
};

#endif
