//
// Created by zhongweiqi on 2025/10/30.
//

#ifndef ATHENA_EVENTDEFS_H
#define ATHENA_EVENTDEFS_H

namespace transport {

enum TriggerEventEnum {
    READ_IDLE,
    WRITE_IDLE,
    ALL_IDLE,
    TIMEOUT,
};

} // namespace transport

#endif //ATHENA_EVENTDEFS_H
