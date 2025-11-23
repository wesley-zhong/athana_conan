#ifndef MSGHANDLER_H_
#define MSGHANDLER_H_


#include "ProtoCommon.pb.h"
#include "ProtoInner.pb.h"
#include "ProtoTask.pb.h"
#include "transport/Channel.h"

class PlayerLoginHandler {
public:
    static void registMsgHandler();

private:
    static void onInnerLoginRes(Channel *channel, InnerLoginResponse *res);

    static void onLoginReq(Channel *channel, LoginRequest *req);

    static void onHeartBeat(Channel *channel, HeartBeatRequest *res);
};

#endif
