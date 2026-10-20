#include <cstring>
#include "ByteBuffer.h"

#include "../log/XLog.h"

namespace core {

//std::shared_ptr<BufferPool>   g_buffPool = std::make_shared<BufferPool>(4096) ;
// constructor
ByteBuffer::ByteBuffer() {
    _storage = createBuffer();
}

// 指定容量：ring 内部会向上取 2 的幂。收包侧可用小容量（溢出即关连接），
// 发包侧必须 ≥ 最大帧长，否则装不下的帧会被静默丢弃
ByteBuffer::ByteBuffer(size_t reserve) {
    _storage = new RingByteBuf(reserve);
}


ByteBuffer::~ByteBuffer() {
    delete _storage;
}

RingByteBuf *ByteBuffer::createBuffer() {
    return new RingByteBuf(4*1024*1024);
}


int ByteBuffer::getNextPackLen() {
    auto readableBytes = (int) _storage->readableBytes();
    if (readableBytes < 8) {
        return -1;
    }

    uint32 packLen = 0;
    _storage->peek(&packLen, 4);
    packLen = Endian::fromNetwork<uint32>(packLen);
   // INFO_LOG("--------------  try get pack len={} all bytes={}", packLen,  readableBytes);
    if (packLen > (uint32) (readableBytes - 4)) {
        return -1;
    }
    return (int) packLen + 4;
}

} // namespace core
