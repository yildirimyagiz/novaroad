/**
 * @file ipc.h
 * @brief Inter-process communication
 */

#ifndef NOVA_IPC_H
#define NOVA_IPC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOVA_CHANNEL_TYPE_DEFINED
#define NOVA_CHANNEL_TYPE_DEFINED
typedef uint64_t nova_channel_t;
#endif

typedef struct {
    uint64_t type;
    uint64_t sender;
    size_t length;
    void *data;
} nova_message_t;

/* ============================================================================
 * Message Channels
 * ========================================================================== */

/**
 * Create IPC channel
 * @return Channel ID or 0 on error
 */
nova_channel_t nova_ipc_channel_create(void);

/**
 * Send message (blocking)
 * @param channel Channel ID
 * @param msg Message to send
 * @return 0 on success, -1 on error
 */
int nova_ipc_channel_send(nova_channel_t channel, const nova_message_t *msg);

/**
 * Receive message (blocking)
 * @param channel Channel ID
 * @param msg Output message
 * @return 0 on success, -1 on error
 */
int nova_ipc_channel_recv(nova_channel_t channel, nova_message_t *msg);

/**
 * Try receive (non-blocking)
 * @param channel Channel ID
 * @param msg Output message
 * @return 0 on success, -1 if no message
 */
int nova_ipc_channel_try_recv(nova_channel_t channel, nova_message_t *msg);

/**
 * Close channel
 * @param channel Channel ID
 */
void nova_ipc_channel_close(nova_channel_t channel);

/* ============================================================================
 * Shared Memory
 * ========================================================================== */

typedef uint64_t nova_shm_t;

/**
 * Create shared memory region
 * @param size Size in bytes
 * @return Shared memory ID or 0 on error
 */
nova_shm_t nova_shm_create(size_t size);

/**
 * Map shared memory into address space
 * @param shm_id Shared memory ID
 * @return Pointer to mapped memory or NULL
 */
void *nova_shm_map(nova_shm_t shm_id);

/**
 * Unmap shared memory
 * @param ptr Mapped pointer
 * @return 0 on success, -1 on error
 */
int nova_shm_unmap(void *ptr);

/**
 * Destroy shared memory
 * @param shm_id Shared memory ID
 * @return 0 on success, -1 on error
 */
int nova_shm_destroy(nova_shm_t shm_id);

/* ============================================================================
 * Signals
 * ========================================================================== */

#define NOVA_SIG_TERM   1
#define NOVA_SIG_KILL   9
#define NOVA_SIG_USR1   10
#define NOVA_SIG_USR2   12

typedef void (*nova_signal_handler_t)(int signum);

/**
 * Send signal to process
 * @param pid Process ID
 * @param signum Signal number
 * @return 0 on success, -1 on error
 */
int nova_signal_send(uint64_t pid, int signum);

/**
 * Set signal handler
 * @param signum Signal number
 * @param handler Handler function
 * @return Previous handler or NULL
 */
nova_signal_handler_t nova_signal_set_handler(int signum, nova_signal_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_IPC_H */
