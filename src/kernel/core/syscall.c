/**
 * @file syscall.c
 * @brief System call dispatcher
 */

#include "kernel/syscall.h"
#include "syscall.h"

typedef int64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static int64_t sys_exit(uint64_t code, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    /* TODO: Implement exit */
    return (int64_t)code;
}

static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a4, uint64_t a5)
{
    (void)fd; (void)buf; (void)count; (void)a4; (void)a5;
    /* TODO: Implement read */
    return -1;
}

static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a4, uint64_t a5)
{
    (void)fd; (void)buf; (void)count; (void)a4; (void)a5;
    /* TODO: Implement write */
    return -1;
}

static syscall_handler_t syscall_table[] = {
    [NOVA_SYS_EXIT]   = sys_exit,
    [NOVA_SYS_READ]   = sys_read,
    [NOVA_SYS_WRITE]  = sys_write,
    /* TODO: Add more syscalls */
};

#define NUM_SYSCALLS (sizeof(syscall_table) / sizeof(syscall_table[0]))

int64_t nova_syscall(uint64_t num, uint64_t arg1, uint64_t arg2,
                     uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    if (num >= NUM_SYSCALLS || syscall_table[num] == NULL) {
        return -1; /* Invalid syscall */
    }
    
    return syscall_table[num](arg1, arg2, arg3, arg4, arg5);
}
