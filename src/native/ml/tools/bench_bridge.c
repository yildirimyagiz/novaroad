#include "nova_bridge.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bench_bridge_flow() {
  uint32_t count = 1000000; // 1M State Elements
  float *server_state = calloc(count, sizeof(float));
  float *server_ref_state =
      calloc(count, sizeof(float)); // What server thinks client has

  NovaBridgeClient *client = nova_bridge_client_create(count);

  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  Nova Bridge (Network Sovereignty) Simulation\n");
  printf("  State Size: %u Floats (%.2f MB)\n", count,
         (count * 4.0) / 1024 / 1024);
  printf("════════════════════════════════════════════════════════════\n\n");

  for (int step = 1; step <= 5; step++) {
    // 1. Update actual server state
    for (uint32_t i = 0; i < count; i++) {
      if (i < 10000)
        server_state[i] = step * 1.1f;
      else if (i < 30000)
        server_state[i] = i * 0.001f + step;
      else if (i % 10000 == 0)
        server_state[i] = rand() / (float)RAND_MAX;
    }

    // 2. Encode using the reference state
    ZMirrorV2Packet *packet =
        zmirror_v2_encode(server_ref_state, server_state, count, 1e-4f);

    // 3. Update server's reference state by applying the SAME packet
    // This ensures server and client never drift
    zmirror_v2_apply(server_ref_state, packet);

    // 4. Serialize & Transmit
    uint32_t wire_size = 0;
    uint8_t *wire_buffer = nova_bridge_serialize(packet, &wire_size);

    // 5. Client receives and updates
    nova_bridge_client_update(client, wire_buffer);

    printf("  [Step %d] Wire Bytes Transmitted: %8u bytes (%.2f KB)\n", step,
           wire_size, wire_size / 1024.0);

    zmirror_v2_destroy_packet(packet);
    free(wire_buffer);
  }

  uint64_t raw_total = (uint64_t)count * sizeof(float) * 5;
  printf("\n  Total Transmitted: %.2f KB\n",
         client->total_received_bytes / 1024.0);
  printf("  Traditional Total: %.2f MB\n", raw_total / 1024.0 / 1024.0);
  printf("  Global Efficiency: %.1f x bandwidth reduction 🚀\n",
         (double)raw_total / client->total_received_bytes);

  int errors = 0;
  for (uint32_t i = 0; i < count; i++) {
    if (fabsf(client->client_state[i] - server_ref_state[i]) > 1e-5f)
      errors++;
  }
  printf("  Final Mirror Sync: %s\n",
         errors == 0 ? "STATE IDENTICAL ✅" : "STATE DRIFT ❌");
  printf("════════════════════════════════════════════════════════════\n\n");

  free(server_state);
  free(server_ref_state);
  free(client->client_state);
  free(client);
}

int main() {
  bench_bridge_flow();
  return 0;
}
