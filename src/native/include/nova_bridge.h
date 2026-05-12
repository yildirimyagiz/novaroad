/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_bridge.h - Sovereign Network Bridge (Mirror-over-Network)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_BRIDGE_H
#define NOVA_BRIDGE_H

#include "nova_mirror_v2.h"
#include <stdint.h>

#define NOVA_BRIDGE_MAGIC 0x48544E5A // 'ZNTH'
#define NOVA_BRIDGE_VERSION 1

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t num_recipes;
  uint32_t patch_size;
  uint32_t total_wire_size;
} NovaBridgeHeader;

// Serializes a Mirror v2 packet into a single binary buffer for network
// transmission
uint8_t *nova_bridge_serialize(const ZMirrorV2Packet *packet,
                                 uint32_t *out_size);

// Deserializes a binary buffer back into a Mirror v2 packet
ZMirrorV2Packet *nova_bridge_deserialize(const uint8_t *buffer);

// Bridge Session (Simulation of a connected client)
typedef struct {
  float *client_state;
  uint32_t state_count;
  uint64_t total_received_bytes;
} NovaBridgeClient;

NovaBridgeClient *nova_bridge_client_create(uint32_t count);
void nova_bridge_client_update(NovaBridgeClient *client,
                                 const uint8_t *wire_buffer);
void nova_bridge_client_destroy(NovaBridgeClient *client);

#endif // NOVA_BRIDGE_H
