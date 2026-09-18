//
// Created by zhongweiqi on 2025/10/29.
//
#include "Dispatcher.h"
#include "log/XLog.h"

namespace transport {

void Dispatcher::processMsg(int msgId, int64_t playerId,Channel* channel, const void *body, int len) {
    auto func = msgMap[msgId];
    if (func == nullptr) {
        INFO_LOG("msg id ={} not found", msgId);
        return;
    }
   // func->msgFunction(playerId, channel, body, len);
}

} // namespace transport
