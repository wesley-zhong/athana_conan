#include <cstring>
#include "ByteBuffer.h"

#include "../log/XLog.h"

//std::shared_ptr<BufferPool>   g_buffPool = std::make_shared<BufferPool>(4096) ;
// constructor
ByteBuffer::ByteBuffer() {
    _storage = createBuffer();
}


ByteBuffer::~ByteBuffer() {
    delete _storage;
}

RingByteBuf *ByteBuffer::createBuffer() {
    return new RingByteBuf(64);
}


int ByteBuffer::getNextPackLen() {
    int readableBytes = _storage->readableBytes();
    if (readableBytes < 8) {
        return -1;
    }

    uint32 packLen = 0;
    _storage->peek(&packLen, 4);
    packLen = Endian::fromNetwork<uint32>(packLen);
   // INFO_LOG("--------------  try get pack len={} all bytes={}", packLen,  readableBytes);
    if (packLen > readableBytes - 4) {
        return -1;
    }
    return packLen + 4;
}
