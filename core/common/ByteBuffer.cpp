#include "ByteBuffer.h"

#include "../log/XLog.h"

namespace core {

// 指定容量：ring 内部会向上取 2 的幂。必须 ≥ 最大帧长，否则装不下的帧会被
// 静默丢弃（writeBytes 返回 false）
ByteBuffer::ByteBuffer(size_t reserve) {
    _storage = new RingByteBuf(reserve);
}


ByteBuffer::~ByteBuffer() {
    delete _storage;
}

} // namespace core
