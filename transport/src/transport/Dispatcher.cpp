//
// Created by zhongweiqi on 2025/10/29.
//
#include "Dispatcher.h"
#include "core/log/XLog.h"

void Dispatcher::processMsg(int msgId, int64_t playerId,Channel* channel, const void *body, int len) {
    auto func = msgMap[msgId];
    if (func == nullptr) {
        INFO_LOG("msg id ={} not found", msgId);
        return;
    }
   // func->msgFunction(playerId, channel, body, len);
}
