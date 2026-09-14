//
// Created by zhongweiqi on 2025/10/28.
//

#ifndef ATHENA_PLAYERNETWORKHANDLLER_H
#define ATHENA_PLAYERNETWORKHANDLLER_H
#include <vector>
#include "core/actor/ActorSystem.h"
#include "transport/EventDefs.h"
struct MsgFunction;
class Channel;


class GatewayServerNetWorkHandler {
public:
    static void initAllMsgRegister();

    static void startLogicThread(int threadNum);

    static void onConnect(Channel *channel);

    static void onMsg(Channel *channel, void *buff, int len);

    static void onClosed(Channel *channel);

    static void onEventTrigger(Channel *channel, TriggerEventEnum reason);

    static void proxyMsgToGame(Channel *channel, char *buff, int len);

    static std::vector<uint64> logicActors; // 逻辑 actor id 列表，onMsg 按 hash 路由
};


#endif //ATHENA_PLAYERNETWORKHANDLLER_H
