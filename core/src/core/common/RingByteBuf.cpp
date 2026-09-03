#include "RingByteBuf.h"
#include <algorithm>
#include <new>

RingByteBuf::RingByteBuf(size_t capacity)
{
    // 防止 capacity + 1 溢出：上取整末尾的 n + 1 会回绕为 0，导致 cap_=0、mask_=SIZE_MAX，
    // 进而 writableBytes() 下溢成 SIZE_MAX，tryWrite 将 memcpy 写穿 0 字节分配
    if (capacity == 0 || capacity > SIZE_MAX / 2)
    {
        throw std::invalid_argument("capacity must be in (0, SIZE_MAX/2]");
    }
    // 自动将用户传入的 capacity 提升为 2 的幂次方（考虑到要浪费1字节，增加1再向上取整）
    cap_ = ceilToPowerOfTwo(capacity + 1);
    mask_ = cap_ - 1;

    buf_ = static_cast<uint8_t*>(je_malloc(cap_));
    if (!buf_)
    {
        throw std::bad_alloc();
    }
}

RingByteBuf::~RingByteBuf()
{
    if (buf_)
    {
        je_free(buf_);
        buf_ = nullptr;
    }
}

// 移动构造函数
RingByteBuf::RingByteBuf(RingByteBuf&& other) noexcept
    : buf_(other.buf_),
      cap_(other.cap_),
      mask_(other.mask_),
      head_(other.head_.load(std::memory_order_relaxed)),
      tail_(other.tail_.load(std::memory_order_relaxed))
{
    other.buf_ = nullptr;
    other.cap_ = 0;
    other.mask_ = 0;
    other.head_.store(0, std::memory_order_relaxed);
    other.tail_.store(0, std::memory_order_relaxed);
}

// 移动赋值运算符
RingByteBuf& RingByteBuf::operator=(RingByteBuf&& other) noexcept
{
    if (this != &other)
    {
        if (buf_)
        {
            je_free(buf_);
        }
        buf_ = other.buf_;
        cap_ = other.cap_;
        mask_ = other.mask_;
        head_.store(other.head_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        tail_.store(other.tail_.load(std::memory_order_relaxed), std::memory_order_relaxed);

        other.buf_ = nullptr;
        other.cap_ = 0;
        other.mask_ = 0;
        other.head_.store(0, std::memory_order_relaxed);
        other.tail_.store(0, std::memory_order_relaxed);
    }
    return *this;
}

size_t RingByteBuf::readableBytes() const noexcept
{
    size_t h = head_.load(std::memory_order_acquire);
    size_t t = tail_.load(std::memory_order_acquire);
    return (t - h) & mask_;
}

size_t RingByteBuf::writableBytes() const noexcept
{
    // moved-from 对象 cap_ == 0，报告不可写，避免 (cap_ - 1) 无符号下溢成 SIZE_MAX
    if (cap_ == 0) return 0;
    // 实际可用最大空间为 cap_ - 1
    return (cap_ - 1) - readableBytes();
}

bool RingByteBuf::tryWrite(const void* src, size_t len)
{
    if (len == 0) return true;
    if (len > writableBytes()) return false;

    size_t t = tail_.load(std::memory_order_relaxed);
    // 计算从 tail_ 到数组末尾的连续空间
    size_t first = std::min(len, cap_ - t);

    std::memcpy(buf_ + t, src, first);
    if (len > first)
    {
        std::memcpy(buf_, static_cast<const uint8_t*>(src) + first, len - first);
    }

    // 利用 mask 进行按位与绕回，等价于 (t + len) % cap_
    tail_.store((t + len) & mask_, std::memory_order_release);
    return true;
}

size_t RingByteBuf::read(void* dst, size_t len)
{
    if (len == 0) return 0;
    size_t avail = readableBytes();
    if (avail == 0) return 0;

    size_t toread = std::min(len, avail);
    size_t h = head_.load(std::memory_order_relaxed);
    size_t first = std::min(toread, cap_ - h);

    std::memcpy(dst, buf_ + h, first);
    if (toread > first)
    {
        std::memcpy(static_cast<uint8_t*>(dst) + first, buf_, toread - first);
    }

    head_.store((h + toread) & mask_, std::memory_order_release);
    return toread;
}

size_t RingByteBuf::peek(void* dst, size_t len) const
{
    if (len == 0) return 0;
    size_t avail = readableBytes();
    if (avail == 0) return 0;

    size_t toread = std::min(len, avail);
    size_t h = head_.load(std::memory_order_relaxed);
    size_t first = std::min(toread, cap_ - h);

    std::memcpy(dst, buf_ + h, first);
    if (toread > first)
    {
        std::memcpy(static_cast<uint8_t*>(dst) + first, buf_, toread - first);
    }
    return toread;
}

uint8_t* RingByteBuf::linearReadablePtr(size_t* outLen) noexcept
{
    size_t avail = readableBytes();
    if (avail == 0)
    {
        *outLen = 0;
        return nullptr;
    }
    size_t h = head_.load(std::memory_order_relaxed);
    // 连续可读区域最多延伸至数组边界
    *outLen = std::min(avail, cap_ - h);
    return buf_ + h;
}

uint8_t* RingByteBuf::linearWritablePtr(size_t* outLen) noexcept
{
    size_t writable = writableBytes();
    if (writable == 0)
    {
        *outLen = 0;
        return nullptr;
    }
    size_t t = tail_.load(std::memory_order_relaxed);
    // 连续可写区域最多延伸至数组边界，不修改内部索引状态
    *outLen = std::min(writable, cap_ - t);
    return buf_ + t;
}

void RingByteBuf::advanceReadIndex(size_t n)
{
    if (n == 0) return;
    if (n > readableBytes()) throw std::out_of_range("advanceReadIndex overflow");
    size_t h = head_.load(std::memory_order_relaxed);
    head_.store((h + n) & mask_, std::memory_order_release);
}

void RingByteBuf::advanceWriteIndex(size_t n)
{
    if (n == 0) return;
    if (n > writableBytes()) throw std::out_of_range("advanceWriteIndex overflow");
    size_t t = tail_.load(std::memory_order_relaxed);
    tail_.store((t + n) & mask_, std::memory_order_release);
}

// 注意：仅可在无生产者/消费者并发访问时调用（两次独立 store 非原子，
// 与在飞的 tryWrite/read 交错的的行为未定义）
void RingByteBuf::clear() noexcept
{
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
}

void* RingByteBuf::je_malloc(size_t size)
{
#ifdef USE_MIMALLOC
    return mi_malloc(size);
#elif defined(USE_JEMALLOC)
    return je_mallocx(size, 0);
#else
    return std::malloc(size);
#endif
}

void RingByteBuf::je_free(void* ptr)
{
#ifdef USE_MIMALLOC
    mi_free(ptr);
#elif defined(USE_JEMALLOC)
    je_free(ptr);
#else
    std::free(ptr);
#endif
}
