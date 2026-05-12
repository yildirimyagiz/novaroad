/**
 * @file actor.h
 * @brief Actor model implementation (message-passing concurrency)
 */

#ifndef NOVA_ACTOR_H
#define NOVA_ACTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nova_actor nova_actor_t;
typedef uint64_t nova_actor_id_t;

typedef void (*nova_actor_free_func_t)(void *data);

/**
 * Actor message structure
 */
typedef struct {
    uint32_t type;
    nova_actor_id_t sender;
    void *data;
    size_t size;
    nova_actor_free_func_t free_func;
} nova_actor_message_t;

/**
 * Actor handler function
 */
typedef void (*nova_actor_handler_t)(nova_actor_message_t *msg, void *user_data);

/**
 * Actor statistics
 */
typedef struct {
    nova_actor_id_t id;
    size_t messages_processed;
    size_t messages_queued;
    size_t mailbox_size;
    uint64_t created_at;
    uint64_t last_message_at;
    bool is_running;
    bool is_processing;
} nova_actor_stats_t;

/* ========================================================================
 * Actor Creation & Lifecycle
 * ======================================================================== */

nova_actor_t *nova_actor_create(const char *name, nova_actor_handler_t handler, void *user_data);
void nova_actor_destroy(nova_actor_t *actor);
void nova_actor_stop(nova_actor_t *actor);

/* ========================================================================
 * Message Passing
 * ======================================================================== */

int nova_actor_send(nova_actor_id_t actor_id, const nova_actor_message_t *message);

/* ========================================================================
 * Actor Info & Stats
 * ======================================================================== */

nova_actor_id_t nova_actor_id(nova_actor_t *actor);
nova_actor_id_t nova_actor_get_id(nova_actor_t *actor);
const char *nova_actor_name(nova_actor_t *actor);
bool nova_actor_is_running(nova_actor_t *actor);
bool nova_actor_is_processing(nova_actor_t *actor);
size_t nova_actor_message_count(nova_actor_t *actor);
nova_actor_stats_t nova_actor_stats(nova_actor_t *actor);

/* ========================================================================
 * Message helpers
 * ======================================================================== */

nova_actor_message_t *nova_actor_message_create(uint32_t type, nova_actor_id_t sender,
                                                const void *data, size_t size,
                                                nova_actor_free_func_t free_func);
void nova_actor_message_destroy(nova_actor_message_t *message);

/* ========================================================================
 * Actor System
 * ======================================================================== */

void nova_actor_system_init(void);
void nova_actor_system_shutdown(void);
size_t nova_actor_count(void);
nova_actor_id_t *nova_actor_list(size_t *count);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_ACTOR_H */
