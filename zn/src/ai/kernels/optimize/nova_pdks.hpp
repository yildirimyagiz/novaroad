#ifndef NOVA_PDKS_HPP
#define NOVA_PDKS_HPP

#include <cmath>
#include <cstdint>

// Nova Hybrid Kernel Engine - Module 6: Precision-Aware Dynamic Kernel
// Selector (PDKS) Groq AI Entegrasyonu: Runtime precision upgrade, enerji için
// INT8 tercih

namespace nova {

typedef struct {
  float max_abs;         // Max absolute value seen
  float min_nonzero_abs; // Smallest nonzero — detects underflow
  uint32_t nan_count;
  uint32_t inf_count;
  float condition_number; // max_abs / min_nonzero_abs
} NumericalHealth;

enum class PrecisionMode { FP32, FP16, INT8 };

// Numerical health scan (SIMD optimized)
void scan_health_avx2(const float *buf, size_t n, NumericalHealth *out);

// Precision Upgrade Policy
template <PrecisionMode Current> struct PrecisionUpgradePolicy {
  static constexpr PrecisionMode evaluate(const NumericalHealth &h) noexcept {
    if constexpr (Current == PrecisionMode::INT8) {
      if (h.condition_number > 1e4f)
        return PrecisionMode::FP16;
    }
    if constexpr (Current == PrecisionMode::FP16) {
      if (h.max_abs > 6.5e4f)
        return PrecisionMode::FP32;
      if (h.nan_count > 0)
        return PrecisionMode::FP32;
      if (h.condition_number > 1e6f)
        return PrecisionMode::FP32;
    }
    return Current; // No upgrade needed
  }
};

// Dynamic Selector
class PrecisionSelector {
public:
  PrecisionMode select(const float *data, size_t n, PrecisionMode current) {
    NumericalHealth h;
    scan_health_avx2(data, n, &h);
    return PrecisionUpgradePolicy<PrecisionMode::INT8>::evaluate(
        h); // Start from INT8
  }

  // Groq AI: Enerji için INT8 tercih et
  PrecisionMode groq_energy_select(PrecisionMode current) {
    // Enerji tasarrufu için INT8'i zorla
    if (current == PrecisionMode::FP32)
      return PrecisionMode::INT8;
    return current;
  }
};

} // namespace nova

#endif // NOVA_PDKS_HPP
