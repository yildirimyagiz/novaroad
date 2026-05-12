/**
 * @file syscall.h
 * @brief System call interface
 */

#ifndef NOVA_SYSCALL_H
#define NOVA_SYSCALL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* System call numbers */
#define NOVA_SYS_EXIT           0
#define NOVA_SYS_FORK           1
#define NOVA_SYS_READ           2
#define NOVA_SYS_WRITE          3
#define NOVA_SYS_OPEN           4
#define NOVA_SYS_CLOSE          5
#define NOVA_SYS_WAIT           6
#define NOVA_SYS_EXEC           7
#define NOVA_SYS_MMAP           8
#define NOVA_SYS_MUNMAP         9
#define NOVA_SYS_SEND           10
#define NOVA_SYS_RECV           11
#define NOVA_SYS_YIELD          12
#define NOVA_SYS_SLEEP          13
#define NOVA_SYS_GETPID         14
#define NOVA_SYS_KILL           15

/**
 * System call dispatcher
 * @param num Syscall number
 * @param arg1-arg5 Arguments
 * @return Syscall return value
 */
int64_t nova_syscall(uint64_t num, uint64_t arg1, uint64_t arg2, 
                     uint64_t arg3, uint64_t arg4, uint64_t arg5);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_SYSCALL_H */
