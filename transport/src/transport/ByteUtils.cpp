//
// Created by zhongweiqi on 2025/10/28.
//

#include "ByteUtils.h"

#include "core/common/ByteConverter.h"


uint32 ByteUtils::readInt32(void *body) {
    int32 value = 0;
    memcpy(&value, body, 4);
    return Endian::fromNetwork32(value);
}

void ByteUtils::writeInt32(void *body, uint32 value) {
    int32 nValue = Endian::toNetwork32(value);
    memcpy(body, &nValue, 4);
}

uint16 ByteUtils::readInt16(void *body) {
    int16 value = 0;
    memcpy(&value, body, 2);
    return Endian::fromNetwork16(value);
}

void ByteUtils::writeInt16(void *body, uint16 value) {
    int16 nValue = Endian::toNetwork16(value);
    memcpy(body, &nValue, 4);
}
