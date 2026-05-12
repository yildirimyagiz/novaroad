/**
 * @file config.h
 * @brief Nova compile-time configuration (simplified for Makefile.simple)
 */

#ifndef NOVA_CONFIG_H
#define NOVA_CONFIG_H

#define NOVA_VERSION_MAJOR 0
#define NOVA_VERSION_MINOR 1
#define NOVA_VERSION_PATCH 0

/* Platform detection */
#define NOVA_PLATFORM_MACOS 1
#define NOVA_PLATFORM_POSIX 1
#define NOVA_PLATFORM_NAME "macos"

/* Desktop platforms */
#define NOVA_PLATFORM_DESKTOP 1

/* Compiler detection */
#define NOVA_COMPILER_CLANG 1

/* Architecture specific */
#define NOVA_ARCH_X86_64 1

#endif /* NOVA_CONFIG_H */
