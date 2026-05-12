/**
 * @file ipc.c
 * @brief Inter-process communication implementation
 */

#include "kernel/ipc.h"
#include "kernel/memory.h"
#include "ipc.h"

#define MAX_CHANNELS 256
#define CHANNEL_QUEUE_SIZE 64

typedef struct {
    nova_message_t queue[CHANNEL_QUEUE_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    bool active;
} channel_data_t;

static channel_data_t channels[MAX_CHANNELS];
static nova_channel_t next_channel_id = 1;

nova_channel_t nova_channel_create(void)
{
    for (size_t i = 0; i < MAX_CHANNELS; i++) {
        if (!channels[i].active) {
            channels[i].head = 0;
            channels[i].tail = 0;
            channels[i].count = 0;
            channels[i].active = true;
            return next_channel_id++;
        }
    }
    return 0; /* No available channels */
}

int nova_channel_send(nova_channel_t channel, const nova_message_t *msg)
{
    if (channel == 0 || channel >= MAX_CHANNELS) return -1;
    
    channel_data_t *ch = &channels[channel - 1];
    if (!ch->active || ch->count >= CHANNEL_QUEUE_SIZE) return -1;
    
    ch->queue[ch->tail] = *msg;
    ch->tail = (ch->tail + 1) % CHANNEL_QUEUE_SIZE;
    ch->count++;
    
    return 0;
}

int nova_channel_recv(nova_channel_t channel, nova_message_t *msg)
{
    if (channel == 0 || channel >= MAX_CHANNELS) return -1;
    
    channel_data_t *ch = &channels[channel - 1];
    if (!ch->active) return -1;
    
    /* Block until message available */
    while (ch->count == 0) {
        /* TODO: Implement blocking wait */
    }
    
    *msg = ch->queue[ch->head];
    ch->head = (ch->head + 1) % CHANNEL_QUEUE_SIZE;
    ch->count--;
    
    return 0;
}

int nova_channel_try_recv(nova_channel_t channel, nova_message_t *msg)
{
    if (channel == 0 || channel >= MAX_CHANNELS) return -1;
    
    channel_data_t *ch = &channels[channel - 1];
    if (!ch->active || ch->count == 0) return -1;
    
    *msg = ch->queue[ch->head];
    ch->head = (ch->head + 1) % CHANNEL_QUEUE_SIZE;
    ch->count--;
    
    return 0;
}

void nova_channel_close(nova_channel_t channel)
{
    if (channel > 0 && channel < MAX_CHANNELS) {
        channels[channel - 1].active = false;
    }
}
