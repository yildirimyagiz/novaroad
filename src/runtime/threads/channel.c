/**
 * @file channel.c
 * @brief Go-style channel implementation with select support
 */

#include "runtime/thread.h"
#include "std/alloc.h"
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHANNEL_CLOSED 1
#define CHANNEL_OPEN 0

// Channel states
typedef enum {
    CHANNEL_STATE_OPEN,
    CHANNEL_STATE_CLOSED,
    CHANNEL_STATE_SELECT_WAIT
} channel_state_t;

// Wait queue entry for select()
typedef struct wait_entry {
    pthread_cond_t *cond;
    int *selected;
    size_t *selected_index;
    struct wait_entry *next;
} wait_entry_t;

// Channel structure
typedef struct {
    void **buffer;   // Circular buffer
    size_t capacity; // Buffer capacity (0 = unbuffered)
    size_t size;     // Current number of items
    size_t head;     // Read position
    size_t tail;     // Write position

    channel_state_t state; // Channel state

    // Synchronization
    pthread_mutex_t mutex;
    pthread_cond_t not_empty; // Signaled when buffer has items
    pthread_cond_t not_full;  // Signaled when buffer has space
    pthread_cond_t closed;    // Signaled when channel is closed

    // Select support
    wait_entry_t *wait_queue; // Waiters for select()

    // Statistics
    atomic_size_t sends;
    atomic_size_t recvs;
    atomic_size_t blocks;
} channel_impl_t;

struct nova_channel {
    channel_impl_t impl;
};

nova_channel_t *nova_channel_create(size_t capacity)
{
    nova_channel_t *ch = nova_alloc(sizeof(nova_channel_t));
    if (!ch)
        return NULL;

    memset(ch, 0, sizeof(nova_channel_t));

    ch->impl.capacity = capacity;
    ch->impl.state = CHANNEL_STATE_OPEN;

    if (capacity > 0) {
        ch->impl.buffer = nova_alloc(sizeof(void *) * capacity);
        if (!ch->impl.buffer) {
            nova_free(ch);
            return NULL;
        }
    }

    // Initialize synchronization primitives
    pthread_mutex_init(&ch->impl.mutex, NULL);
    pthread_cond_init(&ch->impl.not_empty, NULL);
    pthread_cond_init(&ch->impl.not_full, NULL);
    pthread_cond_init(&ch->impl.closed, NULL);

    atomic_init(&ch->impl.sends, 0);
    atomic_init(&ch->impl.recvs, 0);
    atomic_init(&ch->impl.blocks, 0);

    return ch;
}

void nova_channel_destroy(nova_channel_t *ch)
{
    if (!ch)
        return;

    pthread_mutex_lock(&ch->impl.mutex);

    // Close channel first
    ch->impl.state = CHANNEL_STATE_CLOSED;

    // Wake up all waiters
    pthread_cond_broadcast(&ch->impl.not_empty);
    pthread_cond_broadcast(&ch->impl.not_full);
    pthread_cond_broadcast(&ch->impl.closed);

    // Clean up wait queue
    wait_entry_t *entry = ch->impl.wait_queue;
    while (entry) {
        wait_entry_t *next = entry->next;
        nova_free(entry);
        entry = next;
    }

    pthread_mutex_unlock(&ch->impl.mutex);

    // Destroy synchronization primitives
    pthread_mutex_destroy(&ch->impl.mutex);
    pthread_cond_destroy(&ch->impl.not_empty);
    pthread_cond_destroy(&ch->impl.not_full);
    pthread_cond_destroy(&ch->impl.closed);

    // Free buffer
    nova_free(ch->impl.buffer);
    nova_free(ch);
}

int nova_channel_send(nova_channel_t *ch, void *value)
{
    if (!ch)
        return -1;

    pthread_mutex_lock(&ch->impl.mutex);

    // Check if channel is closed
    if (ch->impl.state == CHANNEL_STATE_CLOSED) {
        pthread_mutex_unlock(&ch->impl.mutex);
        return -1;
    }

    // For unbuffered channels, wait for receiver
    if (ch->impl.capacity == 0) {
        while (ch->impl.size > 0) { // Wait for receiver to take the value
            atomic_fetch_add(&ch->impl.blocks, 1);
            pthread_cond_wait(&ch->impl.not_empty, &ch->impl.mutex);

            if (ch->impl.state == CHANNEL_STATE_CLOSED) {
                pthread_mutex_unlock(&ch->impl.mutex);
                return -1;
            }
        }
    } else {
        // For buffered channels, wait for space
        while (ch->impl.size >= ch->impl.capacity) {
            atomic_fetch_add(&ch->impl.blocks, 1);
            pthread_cond_wait(&ch->impl.not_full, &ch->impl.mutex);

            if (ch->impl.state == CHANNEL_STATE_CLOSED) {
                pthread_mutex_unlock(&ch->impl.mutex);
                return -1;
            }
        }
    }

    // Send the value
    if (ch->impl.capacity > 0) {
        ch->impl.buffer[ch->impl.tail] = value;
        ch->impl.tail = (ch->impl.tail + 1) % ch->impl.capacity;
    } else {
        // For unbuffered, the value is conceptually "sent" but stored temporarily
        ch->impl.buffer = &value; // Hack: reuse buffer pointer
    }

    ch->impl.size++;
    atomic_fetch_add(&ch->impl.sends, 1);

    // Signal waiting receivers
    pthread_cond_signal(&ch->impl.not_empty);

    // For unbuffered channels, wait for receiver to acknowledge
    if (ch->impl.capacity == 0) {
        while (ch->impl.size > 0) {
            pthread_cond_wait(&ch->impl.not_full, &ch->impl.mutex);
        }
    }

    pthread_mutex_unlock(&ch->impl.mutex);
    return 0;
}

int nova_channel_recv(nova_channel_t *ch, void **value)
{
    if (!ch || !value)
        return -1;

    pthread_mutex_lock(&ch->impl.mutex);

    // Wait for data
    while (ch->impl.size == 0 && ch->impl.state == CHANNEL_STATE_OPEN) {
        atomic_fetch_add(&ch->impl.blocks, 1);
        pthread_cond_wait(&ch->impl.not_empty, &ch->impl.mutex);
    }

    // Check if channel is closed and empty
    if (ch->impl.size == 0 && ch->impl.state == CHANNEL_STATE_CLOSED) {
        pthread_mutex_unlock(&ch->impl.mutex);
        return -1; // Channel closed
    }

    // Receive the value
    if (ch->impl.capacity > 0) {
        *value = ch->impl.buffer[ch->impl.head];
        ch->impl.head = (ch->impl.head + 1) % ch->impl.capacity;
    } else {
        // For unbuffered
        *value = *(void **) ch->impl.buffer;
    }

    ch->impl.size--;
    atomic_fetch_add(&ch->impl.recvs, 1);

    // Signal waiting senders
    pthread_cond_signal(&ch->impl.not_full);

    // For unbuffered channels, acknowledge receipt
    if (ch->impl.capacity == 0) {
        pthread_cond_signal(&ch->impl.not_empty);
    }

    pthread_mutex_unlock(&ch->impl.mutex);
    return 0;
}

int nova_channel_try_send(nova_channel_t *ch, void *value)
{
    if (!ch)
        return -1;

    pthread_mutex_lock(&ch->impl.mutex);

    if (ch->impl.state == CHANNEL_STATE_CLOSED) {
        pthread_mutex_unlock(&ch->impl.mutex);
        return -1;
    }

    // Check if we can send without blocking
    int can_send = 0;
    if (ch->impl.capacity == 0) {
        can_send = (ch->impl.size == 0);
    } else {
        can_send = (ch->impl.size < ch->impl.capacity);
    }

    if (!can_send) {
        pthread_mutex_unlock(&ch->impl.mutex);
        return 0; // Would block
    }

    // Send the value (similar to nova_channel_send)
    if (ch->impl.capacity > 0) {
        ch->impl.buffer[ch->impl.tail] = value;
        ch->impl.tail = (ch->impl.tail + 1) % ch->impl.capacity;
    } else {
        ch->impl.buffer = &value;
    }

    ch->impl.size++;
    atomic_fetch_add(&ch->impl.sends, 1);
    pthread_cond_signal(&ch->impl.not_empty);

    pthread_mutex_unlock(&ch->impl.mutex);
    return 1; // Sent successfully
}

int nova_channel_try_recv(nova_channel_t *ch, void **value)
{
    if (!ch || !value)
        return -1;

    pthread_mutex_lock(&ch->impl.mutex);

    if (ch->impl.size == 0) {
        if (ch->impl.state == CHANNEL_STATE_CLOSED) {
            pthread_mutex_unlock(&ch->impl.mutex);
            return -1; // Closed
        } else {
            pthread_mutex_unlock(&ch->impl.mutex);
            return 0; // Would block
        }
    }

    // Receive the value (similar to nova_channel_recv)
    if (ch->impl.capacity > 0) {
        *value = ch->impl.buffer[ch->impl.head];
        ch->impl.head = (ch->impl.head + 1) % ch->impl.capacity;
    } else {
        *value = *(void **) ch->impl.buffer;
    }

    ch->impl.size--;
    atomic_fetch_add(&ch->impl.recvs, 1);
    pthread_cond_signal(&ch->impl.not_full);

    pthread_mutex_unlock(&ch->impl.mutex);
    return 1; // Received successfully
}

void nova_channel_close(nova_channel_t *ch)
{
    if (!ch)
        return;

    pthread_mutex_lock(&ch->impl.mutex);

    if (ch->impl.state != CHANNEL_STATE_CLOSED) {
        ch->impl.state = CHANNEL_STATE_CLOSED;

        // Wake up all waiters
        pthread_cond_broadcast(&ch->impl.not_empty);
        pthread_cond_broadcast(&ch->impl.not_full);
        pthread_cond_broadcast(&ch->impl.closed);
    }

    pthread_mutex_unlock(&ch->impl.mutex);
}

int nova_channel_is_closed(nova_channel_t *ch)
{
    if (!ch)
        return 1;
    return ch->impl.state == CHANNEL_STATE_CLOSED;
}

size_t nova_channel_len(nova_channel_t *ch)
{
    if (!ch)
        return 0;
    return ch->impl.size;
}

size_t nova_channel_cap(nova_channel_t *ch)
{
    if (!ch)
        return 0;
    return ch->impl.capacity;
}

nova_channel_stats_t nova_channel_stats(nova_channel_t *ch)
{
    nova_channel_stats_t stats = {0};

    if (!ch)
        return stats;

    stats.capacity = ch->impl.capacity;
    stats.size = ch->impl.size;
    stats.is_closed = (ch->impl.state == CHANNEL_STATE_CLOSED);
    stats.sends = atomic_load(&ch->impl.sends);
    stats.recvs = atomic_load(&ch->impl.recvs);
    stats.blocks = atomic_load(&ch->impl.blocks);

    return stats;
}

int nova_channel_select(nova_select_case_t *cases, int count, int *selected_index)
{
    if (!cases || count <= 0 || !selected_index)
        return -1;

    pthread_mutex_t select_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t select_cond = PTHREAD_COND_INITIALIZER;
    int selected = -1;
    size_t selected_idx = 0;

    // Create wait entry
    wait_entry_t wait_entry = {
        .cond = &select_cond, .selected = &selected, .selected_index = &selected_idx, .next = NULL};

    // Add to wait queues of all channels
    for (int i = 0; i < count; i++) {
        nova_channel_t *ch = cases[i].channel;
        if (!ch)
            continue;

        pthread_mutex_lock(&ch->impl.mutex);

        // Check if operation can proceed immediately
        int can_proceed = 0;
        switch (cases[i].dir) {
        case NOVA_SELECT_SEND:
            if (ch->impl.state == CHANNEL_STATE_CLOSED) {
                can_proceed = 0;
            } else if (ch->impl.capacity == 0) {
                can_proceed = (ch->impl.size == 0);
            } else {
                can_proceed = (ch->impl.size < ch->impl.capacity);
            }
            break;
        case NOVA_SELECT_RECV:
            can_proceed = (ch->impl.size > 0 || ch->impl.state == CHANNEL_STATE_CLOSED);
            break;
        default:
            break;
        }

        if (can_proceed) {
            selected = i;
            selected_idx = i;
            pthread_mutex_unlock(&ch->impl.mutex);
            goto done;
        }

        // Add to wait queue
        wait_entry.next = ch->impl.wait_queue;
        ch->impl.wait_queue = &wait_entry;

        pthread_mutex_unlock(&ch->impl.mutex);
    }

    // Wait for one of the operations to be ready
    pthread_mutex_lock(&select_mutex);
    while (selected == -1) {
        pthread_cond_wait(&select_cond, &select_mutex);
    }
    pthread_mutex_unlock(&select_mutex);

done:
    // Remove from wait queues
    for (int i = 0; i < count; i++) {
        nova_channel_t *ch = cases[i].channel;
        if (!ch)
            continue;

        pthread_mutex_lock(&ch->impl.mutex);

        wait_entry_t **entry = &ch->impl.wait_queue;
        while (*entry) {
            if (*entry == &wait_entry) {
                *entry = wait_entry.next;
                break;
            }
            entry = &(*entry)->next;
        }

        pthread_mutex_unlock(&ch->impl.mutex);
    }

    pthread_mutex_destroy(&select_mutex);
    pthread_cond_destroy(&select_cond);

    if (selected >= 0) {
        *selected_index = selected;
        return 0;
    }

    return -1;
}
