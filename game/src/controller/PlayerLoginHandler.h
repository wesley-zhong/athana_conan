#ifndef MSGHANDLER_H_
#define MSGHANDLER_H_
#include "ProtoInner.pb.h"
#include "transport/Channel.h"

class PlayerLoginHandler {
public:
    static void registMsgHandler();

private:
    static void onInnerLogin(transport::Channel *channel, InnerLoginRequest *req);

    static void onPlayerDisconnected(int64_t playerId, InnerPlayerDisconnectRequest *req);
};

#endif
