/**
 * @file intrinsics.h
 * @brief Compiler intrinsics and builtins
 */

#ifndef NOVA_INTRINSICS_H
#define NOVA_INTRINSICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Count leading zeros
 */
static inline int nova_clz(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(x);
#elif defined(_MSC_VER)
    return (int)__lzcnt64(x);
#else
    int count = 0;
    if (x == 0) return 64;
    for (int i = 63; i >= 0; i--) {
        if (x & (1ULL << i)) break;
        count++;
    }
    return count;
#endif
}

/**
 * Count trailing zeros
 */
static inline int nova_ctz(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(x);
#elif defined(_MSC_VER)
    return (int)_tzcnt_u64(x);
#else
    if (x == 0) return 64;
    int count = 0;
    while ((x & 1) == 0) {
        x >>= 1;
        count++;
    }
    return count;
#endif
}

/**
 * Population count (number of 1 bits)
 */
static inline int nova_popcount(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#elif defined(_MSC_VER)
    return (int)__popcnt64(x);
#else
    int count = 0;
    while (x) {
        count += x & 1;
        x >>= 1;
    }
    return count;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* NOVA_INTRINSICS_H */
