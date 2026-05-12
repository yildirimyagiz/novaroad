/**
 * nova_graph_runtime.c - Default runtime stubs for graph/obligation (e.g. device memory limit)
 */
#include "nova_graph_structs.h"

uint64_t get_device_memory_limit(void) {
  return 1024ULL * 1024 * 1024; /* 1 GB default */
}
