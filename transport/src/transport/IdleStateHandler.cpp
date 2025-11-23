//
// Created by zhongweiqi on 2025/10/30.
//

#include "IdleStateHandler.h"
#include "NetInterface.h"
#include "EventDefs.h"
#include "Channel.h"
void IdleStateHandler::onTimer(Channel *channel, uint64 now) {
    if (max_write_idle_time > 0) {
        if (channel->last_send_time + max_write_idle_time < now) {
            triggerEvent(channel, WRITE_IDLE);
        }
    }

    if (max_read_idle_time > 0) {
        if (channel->last_recv_time + max_read_idle_time < now) {
            triggerEvent(channel, READ_IDLE);
        }
    }
}

void IdleStateHandler::triggerEvent(Channel *channel, TriggerEventEnum reason) {
    netInterface->triggerEvent(channel, reason);
}
