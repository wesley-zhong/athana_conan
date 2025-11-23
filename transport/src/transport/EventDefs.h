//
// Created by zhongweiqi on 2025/10/30.
//

#ifndef ATHENA_EVENTDEFS_H
#define ATHENA_EVENTDEFS_H
#include "core/common/BaseType.h"
#include "ByteUtils.h"

enum TriggerEventEnum {
    READ_IDLE,
    WRITE_IDLE,
    ALL_IDLE,
    TIMEOUT,
};

struct PlayerPktHeader {
    uint32 pid;
    int32 sig;

    int decode(void *body) {
        int passed = 0;
        char *cbody = (char *) body;
        pid = ByteUtils::readInt32(cbody);
        passed += 4;
        sig = ByteUtils::readInt32(cbody + passed);
        passed += 4;
        return passed;
    }

    int encode(void *body) {
        int passed = 0;
        char *cbody = (char *) body;
        ByteUtils::writeInt32(cbody, pid);
        passed += 4;
        ByteUtils::writeInt32(cbody + passed, sig);
        passed += 4;
        return passed;
    }
};

struct InnerPktHeader {
    uint32 pid;
    int32 sig;

    int decode(void *body) {
        int passed = 0;
        char *cbody = (char *) body;
        pid = ByteUtils::readInt32(cbody);
        passed += 4;
        sig = ByteUtils::readInt32(cbody + passed);
        passed += 4;
        return passed;
    }

    int encode(void *body) {
        int passed = 0;
        char *cbody = (char *) body;
        ByteUtils::writeInt32(cbody, pid);
        passed += 4;
        ByteUtils::writeInt32(cbody + passed, sig);
        passed += 4;
        return passed;
    }
};



#endif //ATHENA_EVENTDEFS_H
