#ifndef MSGHANDLER_H_
#define MSGHANDLER_H_

#include "common/BaseType.h"
#include "ProtoCommon.pb.h"

#include "ProtoTask.pb.h"
#include "transport/Channel.h"


class PlayerLoginHandler {
public:
    static void onLoginRes(transport::Channel *channel, LoginResponse *res);

    static void onHeartBeat(transport::Channel *channel, HeartBeatResponse *res);
};

#endif
