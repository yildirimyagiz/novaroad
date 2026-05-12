/**
 * @file platform.c
 * @brief Platform-specific code
 */

#include "platform/platform.h"
#include <string.h>

int nova_platform_init(void)
{
    return 0;
}

const char *nova_platform_name(void)
{
#if defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(_WIN32)
    return "Windows";
#elif defined(__FreeBSD__)
    return "FreeBSD";
#else
    return "Unknown";
#endif
}

const char *nova_platform_arch(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#elif defined(__riscv) && __riscv_xlen == 64
    return "riscv64";
#elif defined(__wasm__)
    return "wasm32";
#else
    return "unknown";
#endif
}
