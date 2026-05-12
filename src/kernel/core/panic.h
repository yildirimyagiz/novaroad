/**
 * @file panic.h
 * @brief Kernel panic internal definitions
 */

#ifndef NOVA_KERNEL_PANIC_H
#define NOVA_KERNEL_PANIC_H

void nova_panic(const char *msg) __attribute__((noreturn));

#endif /* NOVA_KERNEL_PANIC_H */
