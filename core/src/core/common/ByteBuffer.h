#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include "ByteConverter.h"
#include  "BaseType.h"
#include "RingByteBuf.h"
#include "core/log/XLog.h"

class ByteBuffer {
public:
    static size_t const DEFAULT_SIZE = 0x1000;

    // constructor
    ByteBuffer();

    ByteBuffer(size_t reserve);

    ByteBuffer(ByteBuffer &&buf);

    ByteBuffer(ByteBuffer const &right);

    virtual ~ByteBuffer();

    ByteBuffer &operator=(ByteBuffer const &right);

    ByteBuffer &operator=(ByteBuffer &&right);

    RingByteBuf *createBuffer();

    void swapBuffer(ByteBuffer &right);

    RingByteBuf &storage() { return *_storage; }


    void writeInt8(uint8 value) const {
        _storage->write(&value, 1);
    }

    uint8 getInt8() const {
        uint8 value = 0;
        // peek 返回的是实际拷贝的字节数，不是读到的值
        if (_storage->peek(&value, sizeof(value)) != sizeof(value)) return 0;
        return value;
    }

    void writeInt16(uint16 value) const {
        value = Endian::toNetwork<uint16>(value);
        _storage->write(&value, sizeof(value));
    }

    uint16 getInt16() const {
        uint16 value = 0;
        // peek 返回的是实际拷贝的字节数，不是读到的值
        if (_storage->peek(&value, sizeof(value)) != sizeof(value)) return 0;
        return Endian::fromNetwork<uint16>(value);
    }

    void writeInt32(uint32 value) const {
        value = Endian::toNetwork<uint32>(value);
        _storage->write(&value, sizeof(value));
    }

    uint32 getInt32() const {
        uint32 value = 0;
        // peek 返回的是实际拷贝的字节数，不是读到的值
        if (_storage->peek(&value, sizeof(value)) != sizeof(value)) return 0;
        return Endian::fromNetwork<uint32>(value);
    }

    void writeInt64(uint64 value) const {
        value = Endian::toNetwork<uint64>(value);
        _storage->write(&value, sizeof(value));
    }

    uint8 *linearReadablePtr(size_t *outLen) {
        return _storage->linearReadablePtr(outLen);
    }

    uint8 *linearWriteablePtr(size_t *outLen) {
        return _storage->linearWritablePtr(outLen);
    }

    uint64 getInt64() const {
        uint64 value = 0;
        // peek 返回的是实际拷贝的字节数，不是读到的值
        if (_storage->peek(&value, sizeof(value)) != sizeof(value)) return 0;
        return Endian::fromNetwork<uint64>(value);
    }

    bool writeBytes(char *body, size_t size) const {
      bool  ret =  _storage->write(body, size);
      if(!ret){
          ERR_LOG("XXXXXXXXXXXXXXXXXXXX  send buff is full");
      }
        return  ret;
    }

    size_t readBytes(char *body, size_t size) const {
        return _storage->read(body, size);
    }

    void advanceWriteIndex(size_t amount) {
        return _storage->advanceWriteIndex(amount);
    }

    int getNextPackLen();

private:
    RingByteBuf *_storage;
};

#endif
