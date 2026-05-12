#ifndef NOVA_CLO_HPP
#define NOVA_CLO_HPP

#include <cstddef>

// Nova Hybrid Kernel Engine - Module 4: Cache Locality Oracle (CLO)
// Groq AI Entegrasyonu: Reuse distance ile delta processing optimizasyonu

namespace nova {

typedef struct {
  size_t working_set_bytes;
  uint32_t l1_capacity; // typically 32–64 KB
  uint32_t l2_capacity; // typically 256 KB – 1 MB
  uint32_t l3_capacity; // typically 8–64 MB
} CacheSizes;

typedef enum { REUSE_L1 = 0, REUSE_L2, REUSE_L3, REUSE_DRAM } ReuseLevel;

static inline ReuseLevel classify_reuse(const CacheSizes *cs, size_t ws_bytes) {
  if (ws_bytes <= cs->l1_capacity)
    return REUSE_L1;
  if (ws_bytes <= cs->l2_capacity)
    return REUSE_L2;
  if (ws_bytes <= cs->l3_capacity)
    return REUSE_L3;
  return REUSE_DRAM;
}

// Fusion Decision
enum class FusionDecision { ACCEPT, REJECT };

// Fusion Engine with CLO
template <typename OpA, typename OpB> struct FusionEngine {
  static constexpr FusionDecision evaluate(size_t shape_numel, size_t ws_a,
                                           size_t ws_b) noexcept {
    CacheSizes cs{64 * 1024, 512 * 1024, 8 * 1024 * 1024}; // Example
    size_t combined_ws = ws_a + ws_b;
    ReuseLevel reuse = classify_reuse(&cs, combined_ws);

    // Fusion beneficial only when combined working set fits in L2
    if (reuse > REUSE_L2 && shape_numel > 1000000) // LARGE_THRESHOLD
      return FusionDecision::REJECT; // Would evict itself from cache

    // Simulate cost
    size_t cost_fused = 100; // placeholder
    size_t cost_unfused = 150;
    size_t launch_overhead = 10;

    return (cost_fused < cost_unfused + launch_overhead)
               ? FusionDecision::ACCEPT
               : FusionDecision::REJECT;
  }
};

// Groq AI: Delta processing ile ws hesapla
size_t groq_estimate_working_set_with_deltas(size_t original_ws,
                                             int delta_count) {
  // Delta'lar enerji tasarrufu sağlar, ws'yi azalt
  return original_ws - (delta_count * 64); // 64B per delta
}

} // namespace nova

#endif // NOVA_CLO_HPP
