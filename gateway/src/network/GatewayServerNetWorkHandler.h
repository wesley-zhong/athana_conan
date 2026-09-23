//
// Created by zhongweiqi on 2025/10/28.
//

#ifndef ATHENA_PLAYERNETWORKHANDLLER_H
#define ATHENA_PLAYERNETWORKHANDLLER_H
#include <vector>
#include "actor/ActorSystem.h"
#include "transport/EventDefs.h"
namespace transport { struct MsgFunction; }
namespace transport { class Channel; }


class GatewayServerNetWorkHandler {
public:
    static void initAllMsgRegister();

    static void startLogicThread(int ioThreadNum, int logicThreadNum);

    static void onConnect(transport::Channel *channel);

    static void onMsg(transport::Channel *channel, void *buff, int len);

    static void onClosed(transport::Channel *channel);

    static void onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason);

    static void proxyMsgToGame(transport::Channel *channel, char *buff, int len);
};


#endif //ATHENA_PLAYERNETWORKHANDLLER_H
