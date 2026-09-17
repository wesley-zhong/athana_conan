//
// Created by zhongweiqi on 2025/10/28.
//

#pragma once
#include <vector>
#include "core/actor/ActorSystem.h"
#include "transport/EventDefs.h"

namespace transport { struct MsgFunction; }
namespace transport { class Channel; }


class ClientNetWorkHandler {
public:
    static void initAllMsgRegister();

    static void startThread(int threadNum);

    static void onConnect(transport::Channel *channel, int status);

    static void onMsg(transport::Channel *channel, void *buff, int len);

    static void onClosed(transport::Channel *channel);

    static void onEventTrigger(transport::Channel *channel, transport::TriggerEventEnum reason);

    static std::vector<uint64> logicActors; // 逻辑 actor id 列表，onMsg 按 hash 路由
};
