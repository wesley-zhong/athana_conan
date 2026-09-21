//
// Created by zhongweiqi on 2025/10/30.
//

#ifndef ATHENA_EVENTTRIGGER_H
#define ATHENA_EVENTTRIGGER_H
#include "common/BaseType.h"
#include "EventDefs.h"

namespace transport {
class Channel;

class EventTrigger {
public:
    virtual ~EventTrigger() = default;

    virtual void onTimer(Channel *channel, uint64 now) =0;

    virtual void triggerEvent(Channel *channel, TriggerEventEnum reason) = 0;

    // 心跳定时器周期(ms)，由 EventLoop::startHeartbeatTimer 使用
    virtual uint64 timerIntervalMs() const {
        return 5000;
    }
};


} // namespace transport

#endif //ATHENA_EVENTTRIGGER_H
