/**
 * @file actor.c
 * @brief Actor model implementation with message passing and mailboxes
 */

#include "runtime/actor.h"
#include "runtime/async.h"
#include "std/alloc.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ACTOR_MAILBOX_SIZE 1024
#define MAX_ACTORS 10000

typedef struct actor_message_node {
    nova_actor_message_t message;
    struct actor_message_node *next;
} actor_message_node_t;

typedef struct {
    actor_message_node_t *head;
    actor_message_node_t *tail;
    atomic_size_t size;
    atomic_size_t capacity;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} actor_mailbox_t;

struct nova_actor {
    nova_actor_id_t id;
    char *name;
    nova_actor_handler_t handler;
    actor_mailbox_t mailbox;
    atomic_bool running;
    atomic_bool processing;
    pthread_t thread;
    void *user_data;

    // Statistics
    atomic_size_t messages_processed;
    atomic_size_t messages_queued;
    uint64_t created_at;
    uint64_t last_message_at;
};

static struct {
    nova_actor_t **actors;
    size_t num_actors;
    size_t capacity;
    atomic_size_t next_id;
    pthread_mutex_t registry_mutex;
    pthread_rwlock_t actors_rwlock;
} actor_registry;

static void actor_mailbox_init(actor_mailbox_t *mailbox, size_t capacity)
{
    mailbox->head = NULL;
    mailbox->tail = NULL;
    atomic_init(&mailbox->size, 0);
    atomic_init(&mailbox->capacity, capacity);
    pthread_mutex_init(&mailbox->mutex, NULL);
    pthread_cond_init(&mailbox->not_empty, NULL);
    pthread_cond_init(&mailbox->not_full, NULL);
}

static void actor_mailbox_destroy(actor_mailbox_t *mailbox)
{
    // Free all queued messages
    actor_message_node_t *node = mailbox->head;
    while (node) {
        actor_message_node_t *next = node->next;
        if (node->message.data && node->message.free_func) {
            node->message.free_func(node->message.data);
        }
        nova_free(node);
        node = next;
    }

    pthread_mutex_destroy(&mailbox->mutex);
    pthread_cond_destroy(&mailbox->not_empty);
    pthread_cond_destroy(&mailbox->not_full);
}

static bool actor_mailbox_enqueue(actor_mailbox_t *mailbox, const nova_actor_message_t *message)
{
    actor_message_node_t *node = nova_alloc(sizeof(actor_message_node_t));
    if (!node)
        return false;

    // Deep copy message
    node->message.type = message->type;
    node->message.sender = message->sender;
    node->message.size = message->size;
    node->message.free_func = message->free_func;

    if (message->size > 0 && message->data) {
        node->message.data = nova_alloc(message->size);
        if (!node->message.data) {
            nova_free(node);
            return false;
        }
        memcpy(node->message.data, message->data, message->size);
    } else {
        node->message.data = NULL;
    }

    node->next = NULL;

    pthread_mutex_lock(&mailbox->mutex);

    // Wait if mailbox is full
    while (atomic_load(&mailbox->size) >= atomic_load(&mailbox->capacity)) {
        pthread_cond_wait(&mailbox->not_full, &mailbox->mutex);
    }

    if (mailbox->tail) {
        mailbox->tail->next = node;
    } else {
        mailbox->head = node;
    }
    mailbox->tail = node;

    atomic_fetch_add(&mailbox->size, 1);
    pthread_cond_signal(&mailbox->not_empty);

    pthread_mutex_unlock(&mailbox->mutex);
    return true;
}

static bool actor_mailbox_dequeue(actor_mailbox_t *mailbox, nova_actor_message_t *message)
{
    pthread_mutex_lock(&mailbox->mutex);

    while (atomic_load(&mailbox->size) == 0) {
        pthread_cond_wait(&mailbox->not_empty, &mailbox->mutex);
    }

    actor_message_node_t *node = mailbox->head;
    if (!node) {
        pthread_mutex_unlock(&mailbox->mutex);
        return false;
    }

    mailbox->head = node->next;
    if (!mailbox->head) {
        mailbox->tail = NULL;
    }

    // Copy message to output
    *message = node->message;
    atomic_fetch_sub(&mailbox->size, 1);
    pthread_cond_signal(&mailbox->not_full);

    pthread_mutex_unlock(&mailbox->mutex);

    nova_free(node);
    return true;
}

static nova_actor_t *actor_registry_find(nova_actor_id_t id)
{
    pthread_rwlock_rdlock(&actor_registry.actors_rwlock);

    for (size_t i = 0; i < actor_registry.num_actors; i++) {
        if (actor_registry.actors[i] && actor_registry.actors[i]->id == id) {
            nova_actor_t *actor = actor_registry.actors[i];
            pthread_rwlock_unlock(&actor_registry.actors_rwlock);
            return actor;
        }
    }

    pthread_rwlock_unlock(&actor_registry.actors_rwlock);
    return NULL;
}

static bool actor_registry_add(nova_actor_t *actor)
{
    pthread_mutex_lock(&actor_registry.registry_mutex);

    if (actor_registry.num_actors >= actor_registry.capacity) {
        size_t new_capacity = actor_registry.capacity * 2;
        if (new_capacity == 0)
            new_capacity = 16;

        nova_actor_t **new_actors =
            nova_realloc(actor_registry.actors, new_capacity * sizeof(nova_actor_t *));
        if (!new_actors) {
            pthread_mutex_unlock(&actor_registry.registry_mutex);
            return false;
        }

        actor_registry.actors = new_actors;
        actor_registry.capacity = new_capacity;
    }

    actor_registry.actors[actor_registry.num_actors++] = actor;

    pthread_mutex_unlock(&actor_registry.registry_mutex);
    return true;
}

static void actor_registry_remove(nova_actor_t *actor)
{
    pthread_mutex_lock(&actor_registry.registry_mutex);

    for (size_t i = 0; i < actor_registry.num_actors; i++) {
        if (actor_registry.actors[i] == actor) {
            actor_registry.actors[i] = actor_registry.actors[--actor_registry.num_actors];
            break;
        }
    }

    pthread_mutex_unlock(&actor_registry.registry_mutex);
}

static void *actor_worker_thread(void *arg)
{
    nova_actor_t *actor = (nova_actor_t *) arg;

    while (atomic_load(&actor->running)) {
        nova_actor_message_t message;

        if (actor_mailbox_dequeue(&actor->mailbox, &message)) {
            atomic_store(&actor->processing, true);
            actor->last_message_at = time(NULL);

            // Call actor handler
            actor->handler(&message, actor->user_data);

            // Free message data if needed
            if (message.data && message.free_func) {
                message.free_func(message.data);
            }

            atomic_fetch_add(&actor->messages_processed, 1);
            atomic_store(&actor->processing, false);
        }
    }

    return NULL;
}

static nova_actor_id_t generate_actor_id(void)
{
    static atomic_size_t next_id = 1;
    return atomic_fetch_add(&next_id, 1);
}

nova_actor_t *nova_actor_create(const char *name, nova_actor_handler_t handler, void *user_data)
{
    if (!handler)
        return NULL;

    nova_actor_t *actor = nova_alloc(sizeof(nova_actor_t));
    if (!actor)
        return NULL;

    memset(actor, 0, sizeof(nova_actor_t));

    actor->id = generate_actor_id();
    actor->name = name ? strdup(name) : NULL;
    actor->handler = handler;
    actor->user_data = user_data;
    actor->created_at = time(NULL);
    actor->last_message_at = time(NULL);

    atomic_init(&actor->running, true);
    atomic_init(&actor->processing, false);
    atomic_init(&actor->messages_processed, 0);
    atomic_init(&actor->messages_queued, 0);

    // Initialize mailbox
    actor_mailbox_init(&actor->mailbox, ACTOR_MAILBOX_SIZE);

    // Register actor
    if (!actor_registry_add(actor)) {
        actor_mailbox_destroy(&actor->mailbox);
        if (actor->name)
            nova_free(actor->name);
        nova_free(actor);
        return NULL;
    }

    // Create worker thread
    if (pthread_create(&actor->thread, NULL, actor_worker_thread, actor) != 0) {
        actor_registry_remove(actor);
        actor_mailbox_destroy(&actor->mailbox);
        if (actor->name)
            nova_free(actor->name);
        nova_free(actor);
        return NULL;
    }

    return actor;
}

void nova_actor_destroy(nova_actor_t *actor)
{
    if (!actor)
        return;

    // Stop actor
    atomic_store(&actor->running, false);

    // Wake up thread if waiting
    pthread_cond_broadcast(&actor->mailbox.not_empty);

    // Wait for thread to finish
    pthread_join(actor->thread, NULL);

    // Remove from registry
    actor_registry_remove(actor);

    // Clean up
    actor_mailbox_destroy(&actor->mailbox);
    if (actor->name)
        nova_free(actor->name);
    nova_free(actor);
}

int nova_actor_send(nova_actor_id_t actor_id, const nova_actor_message_t *message)
{
    if (!message)
        return -1;

    nova_actor_t *actor = actor_registry_find(actor_id);
    if (!actor)
        return -1;

    if (!actor_mailbox_enqueue(&actor->mailbox, message)) {
        return -1;
    }

    atomic_fetch_add(&actor->messages_queued, 1);
    return 0;
}

nova_actor_id_t nova_actor_id(nova_actor_t *actor)
{
    return actor ? actor->id : 0;
}

const char *nova_actor_name(nova_actor_t *actor)
{
    return actor ? actor->name : NULL;
}

bool nova_actor_is_running(nova_actor_t *actor)
{
    return actor ? atomic_load(&actor->running) : false;
}

bool nova_actor_is_processing(nova_actor_t *actor)
{
    return actor ? atomic_load(&actor->processing) : false;
}

size_t nova_actor_message_count(nova_actor_t *actor)
{
    return actor ? atomic_load(&actor->mailbox.size) : 0;
}

nova_actor_stats_t nova_actor_stats(nova_actor_t *actor)
{
    nova_actor_stats_t stats = {0};

    if (!actor)
        return stats;

    stats.id = actor->id;
    stats.messages_processed = atomic_load(&actor->messages_processed);
    stats.messages_queued = atomic_load(&actor->messages_queued);
    stats.mailbox_size = atomic_load(&actor->mailbox.size);
    stats.created_at = actor->created_at;
    stats.last_message_at = actor->last_message_at;
    stats.is_running = atomic_load(&actor->running);
    stats.is_processing = atomic_load(&actor->processing);

    return stats;
}

nova_actor_message_t *nova_actor_message_create(uint32_t type, nova_actor_id_t sender,
                                                const void *data, size_t size,
                                                nova_actor_free_func_t free_func)
{
    nova_actor_message_t *message = nova_alloc(sizeof(nova_actor_message_t));
    if (!message)
        return NULL;

    message->type = type;
    message->sender = sender;
    message->size = size;
    message->free_func = free_func;

    if (size > 0 && data) {
        message->data = nova_alloc(size);
        if (!message->data) {
            nova_free(message);
            return NULL;
        }
        memcpy(message->data, data, size);
    } else {
        message->data = NULL;
    }

    return message;
}

void nova_actor_message_destroy(nova_actor_message_t *message)
{
    if (!message)
        return;

    if (message->data && message->free_func) {
        message->free_func(message->data);
    }
    nova_free(message);
}

void nova_actor_system_init(void)
{
    memset(&actor_registry, 0, sizeof(actor_registry));
    pthread_mutex_init(&actor_registry.registry_mutex, NULL);
    pthread_rwlock_init(&actor_registry.actors_rwlock, NULL);
}

void nova_actor_system_shutdown(void)
{
    // Stop all actors
    pthread_mutex_lock(&actor_registry.registry_mutex);

    for (size_t i = 0; i < actor_registry.num_actors; i++) {
        if (actor_registry.actors[i]) {
            nova_actor_destroy(actor_registry.actors[i]);
        }
    }

    nova_free(actor_registry.actors);

    pthread_mutex_unlock(&actor_registry.registry_mutex);
    pthread_mutex_destroy(&actor_registry.registry_mutex);
    pthread_rwlock_destroy(&actor_registry.actors_rwlock);
}

size_t nova_actor_count(void)
{
    pthread_mutex_lock(&actor_registry.registry_mutex);
    size_t count = actor_registry.num_actors;
    pthread_mutex_unlock(&actor_registry.registry_mutex);
    return count;
}

nova_actor_id_t *nova_actor_list(size_t *count)
{
    if (!count)
        return NULL;

    pthread_mutex_lock(&actor_registry.registry_mutex);

    *count = actor_registry.num_actors;
    if (actor_registry.num_actors == 0) {
        pthread_mutex_unlock(&actor_registry.registry_mutex);
        return NULL;
    }

    nova_actor_id_t *ids = nova_alloc(actor_registry.num_actors * sizeof(nova_actor_id_t));
    if (!ids) {
        pthread_mutex_unlock(&actor_registry.registry_mutex);
        *count = 0;
        return NULL;
    }

    for (size_t i = 0; i < actor_registry.num_actors; i++) {
        ids[i] = actor_registry.actors[i]->id;
    }

    pthread_mutex_unlock(&actor_registry.registry_mutex);
    return ids;
}

nova_actor_id_t nova_actor_get_id(nova_actor_t *actor)
{
    return actor ? actor->id : 0;
}

void nova_actor_stop(nova_actor_t *actor)
{
    nova_actor_destroy(actor);
}
