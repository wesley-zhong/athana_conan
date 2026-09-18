//
// Created by zhongweiqi on 2025/10/28.
//

#include "ByteUtils.h"

#include "common/ByteConverter.h"

namespace transport {


uint32 ByteUtils::readInt32(void *body) {
    int32 value = 0;
    memcpy(&value, body, 4);
    return core::Endian::fromNetwork32(value);
}

void ByteUtils::writeInt32(void *body, uint32 value) {
    int32 nValue = core::Endian::toNetwork32(value);
    memcpy(body, &nValue, 4);
}

uint16 ByteUtils::readInt16(void *body) {
    int16 value = 0;
    memcpy(&value, body, 2);
    return core::Endian::fromNetwork16(value);
}

void ByteUtils::writeInt16(void *body, uint16 value) {
    int16 nValue = core::Endian::toNetwork16(value);
    memcpy(body, &nValue, 4);
}

} // namespace transport
