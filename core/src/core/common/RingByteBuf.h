//
// Created by zhongweiqi on 2025/10/27.
//

#ifndef ATHENA_RINGBYTEBUF_H
#define ATHENA_RINGBYTEBUF_H

#pragma once


#include <cstdint>
#include <cstddef>
#include <cstring>
#include <atomic>
#include <mutex>
#include <memory>
#include <stdexcept>
//#include <jemalloc/jemalloc.h>


// RingByteBuf: a thread-safe ring (circular) buffer backed by jemalloc.
// - Fixed capacity allocated by jemalloc at construction time.
// - Thread-safe for multiple readers and multiple writers using internal locks.
// - Provides zero-copy access to the contiguous readable region via linearReadablePtr().
// - Supports common operations: write, read, peek, consume, clear, readableBytes, writableBytes.
// - Designed to interoperate with a higher-level ByteBuf (slice/duplicate).


class RingByteBuf {
public:
    explicit RingByteBuf(size_t capacity);

    ~RingByteBuf();


    // no copy
    RingByteBuf(const RingByteBuf &) = delete;

    RingByteBuf &operator=(const RingByteBuf &) = delete;


    // move allowed
    RingByteBuf(RingByteBuf &&other) noexcept;

    RingByteBuf &operator=(RingByteBuf &&other) noexcept;


    // Capacity and sizes
    size_t capacity() const noexcept;

    size_t readableBytes() const noexcept; // total readable
    size_t writableBytes() const noexcept; // total writable


    // Write data (thread-safe)
    // If there's not enough space, returns false and writes nothing.
    bool tryWrite(const void *src, size_t len);


    // Blocking write (will throw if cannot grow or write)
    // Here, buffer is fixed capacity so blocking write will return false if not enough space.
    bool write(const void *src, size_t len) { return tryWrite(src, len); }

    bool write(int write_index, const void *src, size_t len) const;


    // Read data (thread-safe). Reads up to len bytes into dst; returns actual bytes read.
    size_t read(void *dst, size_t len);

    size_t writeTail();


    // Peek without advancing read index (thread-safe). Copies up to len bytes into dst, returns bytes copied.
    size_t peek(void *dst, size_t len) const;


    // Provide pointer to linear readable region and its length (may be less than readableBytes).
    // This is zero-copy: caller must not modify the memory. After using the region, call consume() to advance.
    uint8_t *linearReadablePtr(size_t *outLen) noexcept;

    uint8_t *linearWritablePtr(size_t *outLen) noexcept;

    // Advance the read index by n bytes (thread-safe)
    void advanceReadIndex(size_t n);

    void advanceWriteIndex(size_t n);


    // Reset indices
    void clear() noexcept;

    void *je_malloc(size_t size);

    void je_free(void *ptr);

private:
    uint8_t *buf_; // base pointer
    const size_t cap_; // total capacity
    std::atomic<size_t> head_; // read index (byte offset from base)
    std::atomic<size_t> tail_; // write index (byte offset from base)


    // For multi-reader/multi-writer safety we'll guard read- and write- critical sections
    // with separate mutexes to allow limited concurrency: reads and writes can proceed in parallel
    // as long as they don't race on the same atomic indices in a conflicting way.
    // Note: This is not lock-free; it's a pragmatic, correct design for many apps.
    mutable std::mutex read_mtx_;
    mutable std::mutex write_mtx_;
};

#endif //ATHENA_RINGBYTEBUF_H
