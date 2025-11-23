//
// Created by zhongweiqi on 2025/10/28.
//

#ifndef ATHENA_NETWORKHANDLER_H
#define ATHENA_NETWORKHANDLER_H
#include "thread/AthenaThreadPool.h"
#include "transport/EventDefs.h"
class Channel;

class GateClientNetWorkHandler {
public:
    static void initAllMsgRegister();

    static void startLogicThread(int threadNum);

    static void onNewConnect(Channel *channel, int status);

    static void onMsg(Channel *channel, void *buff, int len);

    static void onClosed(Channel *channel);

    static void onEventTrigger(Channel *channel, TriggerEventEnum reason);

    static Thread::ThreadPool *threadPool;
};


#endif //ATHENA_NETWORKHANDLER_H
