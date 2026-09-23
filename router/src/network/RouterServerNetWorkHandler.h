//
// Created by zhongweiqi on 2026/9/23.
//

#ifndef ATHENA_ROUTERSERVERNETWORKHANDLER_H
#define ATHENA_ROUTERSERVERNETWORKHANDLER_H
#include <vector>
#include "actor/ActorSystem.h"
#include "transport/EventDefs.h"
namespace transport { struct MsgFunction; }
namespace transport { class Channel; }


class RouterServerNetWorkHandler {
public:
    static void initAllMsgRegister();

    static void startLogicThread(int ioThreadNum, int logicThreadNum);

    static void onConnect(transport::Channel *channel);

    static void onMsg(transport::Channel *channel, void *buff, int len);

    static void onClosed(transport::Channel *channel);

    static void onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason);
};


#endif //ATHENA_ROUTERSERVERNETWORKHANDLER_H
