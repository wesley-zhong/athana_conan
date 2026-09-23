//
// Created by zhongweiqi on 2025/10/28.
//

#ifndef ATHENA_NETWORKHANDLER_H
#define ATHENA_NETWORKHANDLER_H
#include <vector>
#include "actor/ActorSystem.h"
#include "transport/EventDefs.h"
namespace transport { struct MsgFunction; }
namespace transport { class Channel; }


class GameServerNetWorkHandler {
public:
    static void initAllMsgRegister();

    static void startLogicThread(int ioThread,int logicThread, int dbThread);

    static void onNewConnect(transport::Channel *channel);


    static void onMsg(transport::Channel *channel, void *buff, int len);

    static void onClosed(transport::Channel *channel);

    static void onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason);
};
#endif //ATHENA_NETWORKHANDLER_H
