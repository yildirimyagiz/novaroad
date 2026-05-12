#include "nova_bridge.h"
#include <stdlib.h>
#include <string.h>

uint8_t *nova_bridge_serialize(const ZMirrorV2Packet *packet,
                                 uint32_t *out_size) {
  if (!packet)
    return NULL;

  uint32_t recipe_bytes = packet->num_recipes * sizeof(ZMirrorRecipe);
  uint32_t total_size =
      sizeof(NovaBridgeHeader) + recipe_bytes + packet->patch_size;

  uint8_t *buffer = malloc(total_size);
  NovaBridgeHeader *header = (NovaBridgeHeader *)buffer;

  header->magic = NOVA_BRIDGE_MAGIC;
  header->version = NOVA_BRIDGE_VERSION;
  header->num_recipes = packet->num_recipes;
  header->patch_size = packet->patch_size;
  header->total_wire_size = total_size;

  uint8_t *ptr = buffer + sizeof(NovaBridgeHeader);

  // Copy Recipes
  memcpy(ptr, packet->recipes, recipe_bytes);
  ptr += recipe_bytes;

  // Copy Patch Data
  memcpy(ptr, packet->patch_data, packet->patch_size);

  if (out_size)
    *out_size = total_size;
  return buffer;
}

ZMirrorV2Packet *nova_bridge_deserialize(const uint8_t *buffer) {
  const NovaBridgeHeader *header = (const NovaBridgeHeader *)buffer;
  if (header->magic != NOVA_BRIDGE_MAGIC)
    return NULL;

  ZMirrorV2Packet *packet = calloc(1, sizeof(ZMirrorV2Packet));
  packet->num_recipes = header->num_recipes;
  packet->patch_size = header->patch_size;

  packet->recipes = malloc(packet->num_recipes * sizeof(ZMirrorRecipe));
  packet->patch_data = malloc(packet->patch_size);

  const uint8_t *ptr = buffer + sizeof(NovaBridgeHeader);

  uint32_t recipe_bytes = packet->num_recipes * sizeof(ZMirrorRecipe);
  memcpy(packet->recipes, ptr, recipe_bytes);
  ptr += recipe_bytes;

  memcpy(packet->patch_data, ptr, packet->patch_size);

  return packet;
}

NovaBridgeClient *nova_bridge_client_create(uint32_t count) {
  NovaBridgeClient *client = calloc(1, sizeof(NovaBridgeClient));
  client->client_state = calloc(count, sizeof(float));
  client->state_count = count;
  client->total_received_bytes = 0;
  return client;
}

void nova_bridge_client_update(NovaBridgeClient *client,
                                 const uint8_t *wire_buffer) {
  if (!client || !wire_buffer)
    return;

  const NovaBridgeHeader *header = (const NovaBridgeHeader *)wire_buffer;
  client->total_received_bytes += header->total_wire_size;

  ZMirrorV2Packet *packet = nova_bridge_deserialize(wire_buffer);
  if (packet) {
    zmirror_v2_apply(client->client_state, packet);
    zmirror_v2_destroy_packet(packet);
  }
}
