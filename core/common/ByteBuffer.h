#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include "ByteConverter.h"
#include  "BaseType.h"
#include "RingByteBuf.h"
#include "log/XLog.h"

namespace core
{
    class ByteBuffer
    {
    public:
        // RingByteBuf 不可拷贝；按值复制/移动 ByteBuffer 属于误用（此前拷贝/移动/swap 只
        // 声明未定义，一旦有人用即 LNK2019），直接禁掉
        ByteBuffer(const ByteBuffer&) = delete;
        ByteBuffer& operator=(const ByteBuffer&) = delete;
        ByteBuffer(ByteBuffer&&) = delete;
        ByteBuffer& operator=(ByteBuffer&&) = delete;

        // 构造：容量向上取 2 的幂，实际可用 = cap-1。必须 ≥ 最大帧长
        // （MAX_PACKET_SIZE+8），装不下整帧时 write 会失败丢帧
        explicit ByteBuffer(size_t reserve);

        ~ByteBuffer();

        RingByteBuf& storage() { return *_storage; }

        void writeInt8(uint8 value)
        {
            _storage->tryWrite(&value, 1);
        }

        // 注意：peek 语义，只看不消费读指针（Channel::peekNextPackLen 靠 peekInt32 偷看帧长）
        uint8 peekInt8() const
        {
            uint8 value = 0;
            // peek 返回的是实际拷贝的字节数，不是读到的值
            if (_storage->peek(&value, sizeof(value)) != sizeof(value)) return 0;
            return value;
        }

        void writeInt16(uint16 value) const
        {
            value = Endian::toNetwork<uint16>(value);
            _storage->tryWrite(&value, sizeof(value));
        }

        uint16 peekInt16() const
        {
            uint16 value = 0;
            // peek 返回的是实际拷贝的字节数，不是读到的值
            if (_storage->peek(&value, sizeof(value)) != sizeof(value)) return 0;
            return Endian::fromNetwork<uint16>(value);
        }

        void writeInt32(uint32 value) const
        {
            value = Endian::toNetwork<uint32>(value);
            _storage->tryWrite(&value, sizeof(value));
        }

        uint32 peekInt32() const
        {
            uint32 value = 0;
            // peek 返回的是实际拷贝的字节数，不是读到的值
            if (_storage->peek(&value, sizeof(value)) != sizeof(value)) return 0;
            return Endian::fromNetwork<uint32>(value);
        }

        void writeInt64(uint64 value) const
        {
            value = Endian::toNetwork<uint64>(value);
            _storage->tryWrite(&value, sizeof(value));
        }

        uint8* linearReadablePtr(size_t* outLen) const
        {
            return _storage->linearReadablePtr(outLen);
        }

        uint8* linearWriteablePtr(size_t* outLen) const
        {
            return _storage->linearWritablePtr(outLen);
        }

        uint64 peekInt64() const
        {
            uint64 value = 0;
            // peek 返回的是实际拷贝的字节数，不是读到的值
            if (_storage->peek(&value, sizeof(value)) != sizeof(value)) return 0;
            return Endian::fromNetwork<uint64>(value);
        }

        bool writeBytes(const char* body, size_t size) const
        {
            bool ret = _storage->tryWrite(body, size);
            if (!ret)
            {
                ERR_LOG("XXXXXXXXXXXXXXXXXXXX  send buff is full");
            }
            return ret;
        }

        size_t readBytes(char* body, size_t size) const
        {
            return _storage->read(body, size);
        }

        // 溢出返回 false（协议/流已错位），由调用方关连接
        bool advanceWriteIndex(size_t amount) const
        {
            return _storage->advanceWriteIndex(amount);
        }

    private:
        RingByteBuf* _storage;
    };
} // namespace core

#endif
