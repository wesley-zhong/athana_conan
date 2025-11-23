
#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>

// -----------------------------
// 平台端序检测
// -----------------------------
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define SYSTEM_BIG_ENDIAN 1
#else
    #define SYSTEM_BIG_ENDIAN 0
#endif

namespace Endian {

    // =============================
    // 基础字节交换函数
    // =============================
    inline uint16_t swap16(uint16_t v) {
        return static_cast<uint16_t>((v << 8) | (v >> 8));
    }

    inline uint32_t swap32(uint32_t v) {
        return ((v & 0x000000FFu) << 24) |
               ((v & 0x0000FF00u) << 8)  |
               ((v & 0x00FF0000u) >> 8)  |
               ((v & 0xFF000000u) >> 24);
    }

    inline uint64_t swap64(uint64_t v) {
        return ((v & 0x00000000000000FFull) << 56) |
               ((v & 0x000000000000FF00ull) << 40) |
               ((v & 0x0000000000FF0000ull) << 24) |
               ((v & 0x00000000FF000000ull) << 8)  |
               ((v & 0x000000FF00000000ull) >> 8)  |
               ((v & 0x0000FF0000000000ull) >> 24) |
               ((v & 0x00FF000000000000ull) >> 40) |
               ((v & 0xFF00000000000000ull) >> 56);
    }

    // =============================
    // 主机 <-> 网络字节序转换
    // =============================
#if SYSTEM_BIG_ENDIAN
    // Big endian 平台，无需转换
    inline uint16_t toNetwork16(uint16_t v) { return v; }
    inline uint32_t toNetwork32(uint32_t v) { return v; }
    inline uint64_t toNetwork64(uint64_t v) { return v; }

    inline uint16_t fromNetwork16(uint16_t v) { return v; }
    inline uint32_t fromNetwork32(uint32_t v) { return v; }
    inline uint64_t fromNetwork64(uint64_t v) { return v; }
#else
    // Little endian 平台，需要反转字节序
    inline uint16_t toNetwork16(uint16_t v) { return swap16(v); }
    inline uint32_t toNetwork32(uint32_t v) { return swap32(v); }
    inline uint64_t toNetwork64(uint64_t v) { return swap64(v); }

    inline uint16_t fromNetwork16(uint16_t v) { return swap16(v); }
    inline uint32_t fromNetwork32(uint32_t v) { return swap32(v); }
    inline uint64_t fromNetwork64(uint64_t v) { return swap64(v); }
#endif

    // =============================
    // 泛型版本（支持 int16/uint32 等）
    // =============================
    template<typename T>
    inline T toNetwork(T value) {
        static_assert(std::is_integral<T>::value, "toNetwork only supports integer types");
        if constexpr (sizeof(T) == 2) return static_cast<T>(toNetwork16(static_cast<uint16_t>(value)));
        if constexpr (sizeof(T) == 4) return static_cast<T>(toNetwork32(static_cast<uint32_t>(value)));
        if constexpr (sizeof(T) == 8) return static_cast<T>(toNetwork64(static_cast<uint64_t>(value)));
        return value;
    }

    template<typename T>
    inline T fromNetwork(T value) {
        static_assert(std::is_integral<T>::value, "fromNetwork only supports integer types");
        if constexpr (sizeof(T) == 2) return static_cast<T>(fromNetwork16(static_cast<uint16_t>(value)));
        if constexpr (sizeof(T) == 4) return static_cast<T>(fromNetwork32(static_cast<uint32_t>(value)));
        if constexpr (sizeof(T) == 8) return static_cast<T>(fromNetwork64(static_cast<uint64_t>(value)));
        return value;
    }
}