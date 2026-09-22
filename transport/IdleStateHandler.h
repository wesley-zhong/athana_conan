//
// Created by zhongweiqi on 2025/10/30.
//

#ifndef ATHENA_IDLESTATEHANDLER_H
#define ATHENA_IDLESTATEHANDLER_H
#include <algorithm>
#include "common/BaseType.h"

#include "EventTrigger.h"

namespace transport {
class NetInterface;
class Channel;
class IdleStateHandler : public EventTrigger {
public:
    IdleStateHandler(NetInterface *net_interface, uint64 max_write_time,
                     uint64 max_read_time) : netInterface(net_interface), max_write_idle_time(max_write_time),
                                             max_read_idle_time(max_read_time) {
    }

    void triggerEvent(Channel *channel, TriggerEventEnum reason) override;

    void onTimer(Channel *channel, uint64 now) override;

    // 取两个空闲阈值中较小的正值为定时周期
    uint64 timerIntervalMs() const override {
        uint64 interval = 2000;
        if (max_write_idle_time > 0) {
            interval = std::min(interval, max_write_idle_time);
        }
        if (max_read_idle_time > 0) {
            interval = std::min(interval, max_read_idle_time);
        }
        return interval;
    }

private:
    NetInterface *netInterface;
    uint64 max_write_idle_time;
    uint64 max_read_idle_time;
};


} // namespace transport

#endif //ATHENA_IDLESTATEHANDLER_H
