#ifndef MSGHANDLER_H_
#define MSGHANDLER_H_
#include "ProtoInner.pb.h"
#include "transport/Channel.h"

class PlayerLoginHandler {
public:
    static void registMsgHandler();

private:
    static void onInnerLogin(Channel *channel, InnerLoginRequest *req);

    static void onPlayerDisconnected(uint64 playerId, InnerPlayerDisconnectRequest *req);
};

#endif
