#include "RingByteBuf.h"

RingByteBuf::RingByteBuf(size_t capacity)
    : buf_(nullptr), cap_(capacity), head_(0), tail_(0) {
    if (capacity == 0) throw std::invalid_argument("capacity must be > 0");
    buf_ = static_cast<uint8_t *>(je_malloc(capacity));
    if (!buf_) throw std::bad_alloc();
}

RingByteBuf::~RingByteBuf() {
    if (buf_) je_free(buf_);
}


size_t RingByteBuf::readableBytes() const noexcept {
    if (tail_ >= head_) return tail_ - head_;
    return cap_ - head_ + tail_;
}

size_t RingByteBuf::writableBytes() const noexcept {
    return cap_ - readableBytes();
}

bool RingByteBuf::tryWrite(const void *src, size_t len) {
    if (len == 0) return true;
    size_t writable = writableBytes();
    if (len > writable) return false;

    if (tail_ + len <= cap_) {
        std::memcpy(buf_ + tail_, src, len);
        tail_ = (tail_ + len) % cap_;
    } else {
        size_t first = cap_ - tail_;
        std::memcpy(buf_ + tail_, src, first);
        std::memcpy(buf_, static_cast<const uint8_t *>(src) + first, len - first);
        tail_ = (tail_ + len) % cap_;
    }
    return true;
}

size_t RingByteBuf::read(void *dst, size_t len) {
    if (len == 0) return 0;
    size_t avail = readableBytes();
    if (avail == 0) return 0;

    size_t toread = std::min(len, avail);
    if (head_ + toread <= cap_) {
        std::memcpy(dst, buf_ + head_, toread);
        head_ = (head_ + toread) % cap_;
    } else {
        size_t first = cap_ - head_;
        std::memcpy(dst, buf_ + head_, first);
        std::memcpy(static_cast<uint8_t *>(dst) + first, buf_, toread - first);
        head_ = (head_ + toread) % cap_;
    }
    return toread;
}

size_t RingByteBuf::peek(void *dst, size_t len) const {
    if (len == 0) return 0;
    size_t avail = readableBytes();
    if (avail == 0) return 0;

    size_t toread = std::min(len, avail);
    if (head_ + toread <= cap_) {
        std::memcpy(dst, buf_ + head_, toread);
    } else {
        size_t first = cap_ - head_;
        std::memcpy(dst, buf_ + head_, first);
        std::memcpy(static_cast<uint8_t *>(dst) + first, buf_, toread - first);
    }
    return toread;
}

uint8_t *RingByteBuf::linearReadablePtr(size_t *outLen) noexcept {
    if (head_ == tail_) {
        *outLen = 0;
        return nullptr;
    }
    if (tail_ > head_) {
        *outLen = tail_ - head_;
        return buf_ + head_;
    }
    *outLen = cap_ - head_;
    return buf_ + head_;
}

uint8_t *RingByteBuf::linearWritablePtr(size_t *outLen) noexcept {
    size_t used = readableBytes();
    if (used == cap_) {
        *outLen = 0;
        return nullptr;
    }

    if (tail_ >= head_) {
        // 如果 tail_ 在末尾，则可以直接写到末尾或 wrap
        *outLen = cap_ - tail_;
        if (*outLen == 0) {
            // wrap 到起始
            *outLen = head_;
            tail_ = 0;
        }
        return buf_ + tail_;
    }
    // wrapped
    *outLen = head_ - tail_;
    return buf_ + tail_;
}

void RingByteBuf::advanceReadIndex(size_t n) {
    if (n == 0) return;
    size_t avail = readableBytes();
    if (n > avail) throw std::out_of_range("consume > readableBytes");
    head_ = (head_ + n) % cap_;
}

void RingByteBuf::advanceWriteIndex(size_t n) {
    if (n == 0) return;
    if (n > writableBytes()) throw std::out_of_range("advanceWriteIndex overflow");
    tail_ = (tail_ + n) % cap_;
}

void RingByteBuf::clear() noexcept {
    head_ = 0;
    tail_ = 0;
}

void *RingByteBuf::je_malloc(size_t size) {
#ifdef USE_MIMALLOC
    return mi_malloc(size);
#elif defined(USE_JEMALLOC)
    return je_mallocx(size, 0);
#else
    return std::malloc(size);
#endif
}

void RingByteBuf::je_free(void *ptr) {
#ifdef USE_MIMALLOC
    mi_free(ptr);
#elif defined(USE_JEMALLOC)
    je_free(ptr);
#else
    std::free(ptr);
#endif
}
