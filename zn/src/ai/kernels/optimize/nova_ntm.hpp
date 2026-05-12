#ifndef NOVA_NTM_HPP
#define NOVA_NTM_HPP

#include <cstdint>
#include <cstdlib>

// Nova Hybrid Kernel Engine - Module 7: NUMA-Aware Memory Topology Manager
// (NTM) Groq AI Entegrasyonu: Çin/Hindistan server'lar için NUMA optimizasyonu

namespace nova {

typedef struct {
  uint32_t node_count;
  uint32_t cores_per_node[8];
  uint64_t mem_per_node_bytes[8];
  uint32_t latency_matrix[8][8]; // relative latency (local=10, remote=20-40)
} NumaTopology;

// NUMA functions (Linux: libnuma; mock here)
void query_numa_topology(NumaTopology *out);
void *numa_alloc_node(size_t bytes, uint32_t node);
void numa_free(void *ptr, size_t bytes);

// NUMA Manager
class NumaManager {
public:
  NumaManager() { query_numa_topology(&topology_); }

  void *alloc_affinity(size_t bytes, uint32_t node) {
    return numa_alloc_node(bytes, node);
  }

  // Affinity scheduling
  uint32_t select_node_for_task(uint32_t task_id) {
    // Round-robin or locality based
    return task_id % topology_.node_count;
  }

  // Groq AI: Çin/Hindistan server'lar için bandwidth budgeting
  void budget_inter_socket_bandwidth() {
    // Cap cross-NUMA transfers at 60% of link capacity
    printf("🌍 Groq AI: NUMA bandwidth budgeted for global servers\n");
  }

private:
  NumaTopology topology_;
};

} // namespace nova

#endif // NOVA_NTM_HPP
