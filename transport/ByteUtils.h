//
// Created by zhongweiqi on 2025/10/28.
//

#ifndef ATHENA_BYTEUTILS_H
#define ATHENA_BYTEUTILS_H
#include "common/BaseType.h"

namespace transport
{
    class ByteUtils
    {
    public:
        // 读接口只读内存，收 const*；写接口才需要可写内存
        static uint32 readInt32(const void* body);

        static void writeInt32(void* body, uint32 value);

        static uint16 readInt16(const void* body);

        static void writeInt16(void* body, uint16 value);
    };
} // namespace transport

#endif //ATHENA_BYTEUTILS_H
