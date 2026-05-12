#ifndef NOVA_CORE_HPP
#define NOVA_CORE_HPP

#include <cstdint>
#include <cstdio>
#include <cstdlib>

/**
 * Nova Sovereign Runtime (ZSR)
 *
 * Nova V4 için yüksek performanslı, deterministik ve
 * düşük seviyeli (low-level) çekirdek kütüphanesidir.
 */

namespace nova {

// Ham bellek yönetimi
template <typename T> class RawMemory {
public:
  T *data;
  size_t size;

  RawMemory(size_t n) : size(n) {
    data = (T *)std::malloc(n * sizeof(T));
    if (!data) {
      std::fprintf(stderr, "Fatal: Out of memory in RawMemory allocation\n");
      std::abort();
    }
  }
  ~RawMemory() { std::free(data); }

  T &operator[](size_t idx) { return data[idx]; }
  const T &operator[](size_t idx) const { return data[idx]; }
};

// Yüksek hassasiyetli zamanlayıcı
struct Clock {
  static uint64_t ticks() {
    // Platform-specific high-performance counters
#if defined(__x86_64__) || defined(_M_X64)
    unsigned int lo, hi;
    // Serialize instruction stream to ensure accurate measurement
    __asm__ __volatile__("lfence; rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    // Read virtual counter - no special serialization needed on ARM usually for
    // simple timing
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    // Fallback: Standard C++ high_resolution_clock
    // Nanoseconds since epoch as a rough equivalent to ticks
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               now.time_since_epoch())
        .count();
#endif
  }
};

} // namespace nova

#endif // NOVA_CORE_HPP
