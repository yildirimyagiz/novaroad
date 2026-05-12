# Nova Native ML — Kaynak Dizini
> 📅 Oluşturuldu: 2026-02-26  
> 📁 Konum: `src/native/ml/`  
> 🎯 Amaç: Nova'nın C/C++ native ML altyapısını tek bir yerde toplamak

---

## 📂 Dizin Yapısı

| Dizin | İçerik | Dosya |
|-------|--------|-------|
| `ai/` | AI core, kernels, models, optim, training, numerics | 55 |
| `backends/` | CPU, CUDA, Metal, ROCm, LLVM, Vulkan, OpenCL, SIMD, Mobile | 49 |
| `compute/` | Graph computing, cognitive scheduler, bridge, crypto, dispatcher | 91 |
| `groq-ai/` | Groq AI entegrasyonu, matmul opt, metal flash, energy manager | 125 |
| `optimizer/` | Adaptive, autotune, kernel fusion, learning runtime, invariants | 19 |
| `quantization/` | Model quantization | 1 |
| `memory/` | Allocator, GPU memory pool, memory planner | 4 |
| `cluster/` | Cluster, economy | 2 |
| `experimental/` | Sovereign actuator bridge | 2 |
| `benches/` | Flash attention, graph ops, matmul, inference, JIT, quantization benchmarks | 6 |
| `tools/` | ML demo, llama bench, scientific validation, JIT demo, training demo | 37 |
| `runtime/` | JIT, GC, parallel, deterministic execution | 9 |

**Toplam:** ~400 dosya

---

## 🗂️ Alt Dizin Detayları

### 🤖 `ai/`
- `core/` — AI temel yapılar
- `kernels/` — Native kernel implementasyonları
- `models/` — Model tanımları
- `numerics/` — Sayısal hesaplama
- `optim/` — Optimizasyon algoritmaları
- `training/` — Eğitim altyapısı
- `io/` — Model I/O
- `tests/` — AI testleri

### ⚡ `backends/`
- `cpu/` — CPU backend (SIMD, AVX, NEON)
- `cuda/` — CUDA GPU backend
- `metal/` — Apple Metal backend (M1/M2/M3)
- `llvm/` — LLVM JIT backend
- `vulkan/` — Vulkan compute
- `opencl/` — OpenCL backend
- `rocm/` — AMD ROCm backend
- `mobile/` — iOS/Android backend
- `simd/` — SIMD primitives
- `hyperflash/` — HyperFlash memory backend
- `cross/` — Cross-platform backend dispatcher

### 🧮 `compute/`
- `graph/` — Compute graph engine
- `groq compute/` — Groq compute integration
- `nova_compute.c` — Ana compute engine
- `nova_cognitive_scheduler.c` — Bilişsel zamanlayıcı
- `nova_dispatcher.c` — İş dağıtıcı
- `nova_bridge.c` — Platform köprüsü

### 🚀 `groq-ai/`
- Metal flash attention benchmark
- Matmul optimizasyonları
- Groq AI entegrasyonu
- Energy manager
- Amazon integration
- Goodhart koruma sistemi

### 🔧 `optimizer/`
- `nova_adaptive_optimizer.c` — Adaptif optimizer
- `nova_autotune.c` — Otomatik tuning
- `nova_kernel_fusion.c` — Kernel fusion
- `nova_learning_runtime.c` — Öğrenen runtime
- `nova_optimizer_supreme.c` — Supreme optimizer
- `fusion/` — Fusion operatörleri

### 📊 `benches/`
- `flash/bench_flash_attention.c`
- `graph/bench_graph_ops.c`
- `kernel/bench_matmul.c`
- `llm/bench_inference.c`
- `llvm/bench_jit_compilation.c`
- `quant/bench_quantization.c`

---

## 🔗 İlgili Dizinler

- `src/native/src/formal/` — Formal doğrulama (ML değil, ayrı tutuldu)
- `src/native/src/compiler/` — Derleyici altyapısı (ML değil)
- `src/native/calibration/` — Kalibrasyon sistemi
- `zn/ml/` — Nova dili ML modülleri (.zn)
- `include/` — Header dosyaları

---

## 📋 Roadmap

Detaylı roadmap için: [`docs/ml/check/ROADMAP_ML_AI.md`](../../../docs/ml/check/ROADMAP_ML_AI.md)
