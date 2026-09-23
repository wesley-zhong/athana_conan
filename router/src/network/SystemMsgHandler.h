//
// Created by zhongweiqi on 2026/9/23.
//

#ifndef ATHENA_SYSTEMMSGHANDLER_H
#define ATHENA_SYSTEMMSGHANDLER_H
#include "ProtoInner.pb.h"
namespace transport { class Channel; }


class SystemMsgHandler {
public:
    static void registMsg();

private:
    static void onShakHandReq(transport::Channel *channel, InnerServerHandShakeReq *req);

    static void onShakHandResponse(transport::Channel *channel, InnerServerHandShakeRes *res);

    static void onInnerHeartBeatReq(transport::Channel *channel, InnerHeartBeatRequest *req);

    static void onInnerHeartBeatRes(transport::Channel *channel, InnerHeartBeatResponse *res);
};


#endif //ATHENA_SYSTEMMSGHANDLER_H
