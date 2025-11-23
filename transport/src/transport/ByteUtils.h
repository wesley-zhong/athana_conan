//
// Created by zhongweiqi on 2025/10/28.
//

#ifndef ATHENA_BYTEUTILS_H
#define ATHENA_BYTEUTILS_H
#include "core/common/BaseType.h"


class ByteUtils {
public:
    static uint32 readInt32(void *body);

    static void writeInt32(void *body, uint32 value);

    static uint16 readInt16(void *body);

    static void writeInt16(void *body, uint16 value);
};


#endif //ATHENA_BYTEUTILS_H
