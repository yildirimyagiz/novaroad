#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

/**
 * Nova V4 Native Runtime Library (z-rt)
 * Bu dosya Nova tarafından üretilen C++ kodlarının temel taşıdır.
 */

namespace z_rt {
// Nova'in Mutation Pulse mantığını simüle eden temel yapı
class MutationPulse {
public:
  static void synchronize() {
    // Memory barrier to ensure visibility of memory operations across threads
    // This is crucial for lock-free data structures and IPC
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }
};

// Zaman ölçümü için yardımcı sınıf
class Timer {
  std::chrono::high_resolution_clock::time_point start_time;

public:
  void start() { start_time = std::chrono::high_resolution_clock::now(); }
  double stop() {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start_time).count();
  }
};
} // namespace z_rt

// Nova'in i32, f64 gibi temel tiplerini C++ tiplerine eşliyoruz
using i32 = int32_t;
using u32 = uint32_t;
using f64 = double;
using str = std::string;

// console.log simülasyonu
namespace console {
template <typename T> void log(T val) { std::cout << val << std::endl; }
} // namespace console
