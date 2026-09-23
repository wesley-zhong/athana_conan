//
// Created by zhongweiqi on 2025/10/28.
//

#ifndef ATHENA_GAME_CLIENTNETWORKHANDLER_H
#define ATHENA_GAME_CLIENTNETWORKHANDLER_H
#include "transport/EventDefs.h"

namespace transport { class Channel; }

class GameClientNetWorkHandler {
public:
    static void initAllMsgRegister();

    static void onNewConnect(transport::Channel *channel, int status);

    static void onMsg(transport::Channel *channel, void *buff, int len);

    static void onClosed(transport::Channel *channel);

    static void onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason);
};


#endif //ATHENA_GAME_CLIENTNETWORKHANDLER_H
