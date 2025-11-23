//
// Created by zhongweiqi on 2025/10/31.
//

#ifndef ATHENA_SYSTEMMSGHANDLER_H
#define ATHENA_SYSTEMMSGHANDLER_H
#include "ProtoInner.pb.h"
class Channel;


class SystemMsgHandler {
public:
    static void registMsg();

private:
    static void onShakHandReq(Channel *channel, InnerServerHandShakeReq *req);

    static void onShakHandResponse(Channel *channel, InnerServerHandShakeRes *res);

    static void onHeartBeatReq(Channel *channel, InnerHeartBeatRequest *req);

    static void onHeartBeatRes(Channel *channel, InnerHeartBeatResponse *res);
};


#endif //ATHENA_SYSTEMMSGHANDLER_H
