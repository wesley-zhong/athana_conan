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

// RingByteBuf: SPSC (Single-Producer Single-Consumer) 场景下高效的环形字节缓冲区。
// - 容量强制/自动调整为 2 的幂次方，使用位运算（& mask）替代取模（% cap）。
// - 实际最大可用容量为 cap_ - 1（浪费 1 字节用于区分空/满）。
class RingByteBuf {
public:
    explicit RingByteBuf(size_t capacity);
    ~RingByteBuf();

    // 禁止拷贝
    RingByteBuf(const RingByteBuf &) = delete;
    RingByteBuf &operator=(const RingByteBuf &) = delete;

    // 允许移动
    RingByteBuf(RingByteBuf &&other) noexcept;
    RingByteBuf &operator=(RingByteBuf &&other) noexcept;

    // 容量与大小
    size_t capacity() const noexcept { return cap_ - 1; } // 返回实际可用最大字节数
    size_t totalAllocatedCapacity() const noexcept { return cap_; } // 返回实际分配的内存大小
    size_t readableBytes() const noexcept;
    size_t writableBytes() const noexcept;

    // 写入与读取
    bool tryWrite(const void *src, size_t len);
    bool write(const void *src, size_t len) { return tryWrite(src, len); }
    size_t read(void *dst, size_t len);
    size_t peek(void *dst, size_t len) const;

    // 零拷贝接口
    uint8_t *linearReadablePtr(size_t *outLen) noexcept;
    uint8_t *linearWritablePtr(size_t *outLen) noexcept;

    // 移动读写指针
    void advanceReadIndex(size_t n);
    void advanceWriteIndex(size_t n);

    // 清空缓冲区
    void clear() noexcept;

    // 内存分配辅助
    static void *je_malloc(size_t size);
    static void je_free(void *ptr);

private:
    // 计算大于等于 n 的最小 2 的幂次方
    static constexpr size_t ceilToPowerOfTwo(size_t n) noexcept {
        if (n <= 2) return n < 1 ? 2 : n;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
        n |= n >> 32;
#endif
        return n + 1;
    }

private:
    uint8_t *buf_{nullptr};            // 内存基址
    size_t cap_{0};                    // 实际分配容量 (必须是 2 的幂)
    size_t mask_{0};                   // 掩码 (cap_ - 1)
    std::atomic<size_t> head_{0};     // 读索引
    std::atomic<size_t> tail_{0};     // 写索引
};

#endif // ATHENA_RINGBYTEBUF_H