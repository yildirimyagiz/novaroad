#ifndef NOVA_WSS_HPP
#define NOVA_WSS_HPP

#include <array>
#include <atomic>
#include <optional>

// Nova Hybrid Kernel Engine - Module 2: Cross-Backend Work Stealing Scheduler
// (WSS) Groq AI Entegrasyonu: Otomatik backend seçimi, enerji dengesi

namespace nova {

enum class Backend { CPU, SIMD, CUDA, Metal };

// Kernel Task
struct KernelTask {
  uint32_t kernel_id;
  void *params;
  Backend preferred_backend;
};

// Lock-free Work Queue per Backend
template <Backend B> struct WorkQueue {
  static constexpr size_t QUEUE_SIZE = 4096;
  alignas(64) std::atomic<uint32_t> head{0};
  alignas(64) std::atomic<uint32_t> tail{0};
  std::array<KernelTask, QUEUE_SIZE> tasks;

  bool try_steal(KernelTask &out) noexcept {
    uint32_t h = head.load(std::memory_order_acquire);
    uint32_t t = tail.load(std::memory_order_acquire);
    if (h == t)
      return false; // Empty
    out = tasks[h % QUEUE_SIZE];
    return head.compare_exchange_weak(h, h + 1, std::memory_order_acq_rel);
  }

  bool push(KernelTask t) noexcept {
    uint32_t t_old = tail.load(std::memory_order_acquire);
    uint32_t h = head.load(std::memory_order_acquire);
    if ((t_old - h) >= QUEUE_SIZE)
      return false; // Full
    tasks[t_old % QUEUE_SIZE] = t;
    tail.store(t_old + 1, std::memory_order_release);
    return true;
  }

  bool pop(KernelTask &out) noexcept {
    uint32_t t = tail.load(std::memory_order_acquire);
    uint32_t h = head.load(std::memory_order_acquire);
    if (h == t)
      return false; // Empty
    out = tasks[(t - 1) % QUEUE_SIZE];
    return tail.compare_exchange_weak(t, t - 1, std::memory_order_acq_rel);
  }
};

// Work Stealing Scheduler Controller
struct WSSController {
  WorkQueue<Backend::CPU> cpu_q;
  WorkQueue<Backend::SIMD> simd_q;
  WorkQueue<Backend::CUDA> cuda_q;
  WorkQueue<Backend::Metal> metal_q;

  void rebalance() noexcept {
    // Scan queues and steal if needed
    steal_if_needed<Backend::CPU>();
    steal_if_needed<Backend::SIMD>();
    steal_if_needed<Backend::CUDA>();
    steal_if_needed<Backend::Metal>();
  }

private:
  template <Backend Target> void steal_if_needed() {
    // Implement steal logic: Check queue depth, affinity, cost
    // Groq AI: Enerji bazlı tercih (ör. CPU'dan SIMD'e steal et enerji için)
    printf("⚡ WSS: Rebalancing for backend %d\n", static_cast<int>(Target));
  }
};

// Global WSS instance
inline WSSController &get_wss() {
  static WSSController wss;
  return wss;
}

// Groq AI: Otomatik backend seçimi
Backend groq_auto_select_backend(const KernelTask &task) {
  // Basitleştirilmiş: Matmul için CUDA, diğer için CPU
  if (task.kernel_id == 1) { // Assume matmul
    return Backend::CUDA;
  }
  return Backend::CPU;
}

} // namespace nova

#endif // NOVA_WSS_HPP
