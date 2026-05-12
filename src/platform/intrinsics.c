/**
 * @file intrinsics.c
 * @brief Compiler intrinsics
 */

#include "platform/intrinsics.h"

int nova_clz(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(x);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse64(&index, x);
    return 63 - index;
#else
    int count = 0;
    while (x && !(x & (1ULL << 63))) {
        x <<= 1;
        count++;
    }
    return count;
#endif
}

int nova_ctz(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(x);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, x);
    return index;
#else
    int count = 0;
    while (x && !(x & 1)) {
        x >>= 1;
        count++;
    }
    return count;
#endif
}

int nova_popcount(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#elif defined(_MSC_VER)
    return __popcnt64(x);
#else
    int count = 0;
    while (x) {
        count += x & 1;
        x >>= 1;
    }
    return count;
#endif
}
