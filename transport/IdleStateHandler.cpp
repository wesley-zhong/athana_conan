//
// Created by zhongweiqi on 2025/10/30.
//

#include "IdleStateHandler.h"
#include "NetInterface.h"
#include "EventDefs.h"
#include "Channel.h"

namespace transport {
void IdleStateHandler::onTimer(Channel *channel, uint64 now) {
    if (max_write_idle_time > 0) {
        if (channel->last_send_time + max_write_idle_time < now) {
            // 触发后重置基线：一个空闲周期只触发一次，与 netty IdleStateHandler 语义一致
            channel->last_send_time = now;
            triggerEvent(channel, WRITE_IDLE);
        }
    }

    if (max_read_idle_time > 0) {
        if (channel->last_recv_time + max_read_idle_time < now) {
            channel->last_recv_time = now;
            triggerEvent(channel, READ_IDLE);
        }
    }
}

void IdleStateHandler::triggerEvent(Channel *channel, TriggerEventEnum reason) {
    netInterface->triggerEvent(channel, reason);
}

} // namespace transport
