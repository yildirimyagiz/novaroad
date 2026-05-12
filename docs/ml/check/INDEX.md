# Nova Language — Feature Roadmap Index
> 📅 Son güncelleme: 2026-02-26  
> 📁 Konum: `docs/ml/check/`  
> 🔗 Kaynak: `zn/` dizini (1928 `.zn` dosyası) + `src/native/src/formal/`

---

## 📊 Genel Durum Özeti

| Dosya | Kapsam | Dosya Sayısı | Boyut |
|-------|--------|-------------|-------|
| [ROADMAP_ML_AI.md](./ROADMAP_ML_AI.md) | ML Core + Neural Nets + Optimizers + GPU Kernels + AI Stdlib | ~354 dosya | 31 KB |
| [ROADMAP_KERNEL_FFI_PLATFORM.md](./ROADMAP_KERNEL_FFI_PLATFORM.md) | Kernel + FFI Bridge + Platform Targets | ~60 dosya | 18 KB |
| [ROADMAP_STDLIB.md](./ROADMAP_STDLIB.md) | Tüm Standard Library (UI, Web, Science, Domain...) | ~709 dosya | 22 KB |
| [ROADMAP_COMPILER_TOOLCHAIN.md](./ROADMAP_COMPILER_TOOLCHAIN.md) | Compiler Pipeline + Toolchain + Tests + Examples | ~359 dosya | 9.7 KB |
| [ROADMAP_SECURITY.md](./ROADMAP_SECURITY.md) | Security + Formal Verification + Crypto + SMT | ~34 dosya | 14 KB |

**Toplam:** 1928+ `.zn` dosyası — 5 kategori — 5 roadmap belgesi

---

## 🗂️ Modüller ve Roadmap Haritası

### 🤖 1. ML / AI — [ROADMAP_ML_AI.md](./ROADMAP_ML_AI.md)
```
zn/ml/          → 58 dosya  (Neural Net, Optimizer, Ensemble, GPU Kernel, AutoML...)
zn/stdlib/ai/   → 296 dosya (Self-Learning, Diffusion, Speech, Domain AI...)
```
**Öne çıkan özellikler:**
- 9 optimizer: Adam, SGD, RMSprop, RAdam, Nadam, Adadelta, Adagrad...
- CUDA + Metal GPU kernel'leri (Flash Attention, matmul)
- XGBoost, LightGBM, Gradient Boosting ensemble
- BERT modeli, AutoML domain detection (Tabular/Vision/NLP/TimeSeries)
- Air/Fuel teknoloji AI modülü

---

### ⚙️ 2. Kernel / FFI / Platform — [ROADMAP_KERNEL_FFI_PLATFORM.md](./ROADMAP_KERNEL_FFI_PLATFORM.md)
```
zn/kernel/      → 36 dosya  (core, drivers, hal, memory, security)
zn/ffi/         → 13 dosya  (C, JS, Python, Rust, Swift, Kotlin köprüler)
zn/platform/    → 11 dosya  (7 platform hedefi + GPU/ANE/SIMD capabilities)
```
**Öne çıkan özellikler:**
- FROZEN ABI: `nova_[module]_[function]` naming convention
- M:N task scheduler (cooperative scheduling)
- 7 platform: macOS Apple Silicon, macOS x86, iOS, Android, Linux, Windows, WASM
- Swift smoke test ✅ + Kotlin JNI smoke test ✅
- Hardware Abstraction Layer: aarch64, x86_64, RISC-V, WASM

---

### 📚 3. Standard Library — [ROADMAP_STDLIB.md](./ROADMAP_STDLIB.md)
```
zn/stdlib/      → 709 dosya (ai, science, ui, domain, algorithms, web, crypto...)
```
**Öne çıkan özellikler:**
- 🎨 **ZenFlow UI Framework**: charts (bar, line, pie, scatter, heatmap) + layout (flex, grid) + 8 widget türü
- 🌐 **Web Framework**: HTTP, router, SSR, reactivity, SEO, sitemap, nova_serve
- 🔬 **Science & Engineering**: 88 science + 20 chemistry + 19 physics = 127 dosya
- 🌍 **Domain**: industrial, education, devops, gaming, electrical engineering

---

### 🔨 4. Compiler & Toolchain — [ROADMAP_COMPILER_TOOLCHAIN.md](./ROADMAP_COMPILER_TOOLCHAIN.md)
```
zn/src/compiler/ → 127 dosya (self-hosting compiler pipeline)
zn/toolchain/    →   8 dosya (znpkg, znup, zndoc, znlint, znrepl, znfmt, zntest)
zn/src/tools/    →  60 dosya (LSP, debugger, profiler...)
zn/tests/        →  55 dosya (unit, integration, benchmarks)
zn/examples/     →   8 dosya
```
**Öne çıkan özellikler:**
- Self-hosting hedefi: Nova compiler Nova ile yazılıyor
- Tam toolchain: formatter + linter + REPL + doc generator + test runner
- 8 örnek: hello_world, ai_inference, formal_proof, llama_mini, microkernel, mobile_app, native_app, web_app

---

### 🔐 5. Security & Formal Verification — [ROADMAP_SECURITY.md](./ROADMAP_SECURITY.md)
```
zn/security/               → 9 dosya  (capability model, sandboxing, audit, supply chain)
zn/stdlib/crypto/          → 16 dosya
src/native/src/formal/     → 16 .c dosyası (CVC5, Isabelle, KLEE, SMT2)
```
**Öne çıkan özellikler:**
- CVC5 SMT solver bridge (15.1 KB)
- Isabelle proof assistant bridge
- KLEE symbolic execution
- Non-interference + information flow formal proofs
- Policy engine (17.8 KB) + Kernel contracts (16.8 KB)

---

## ✅ Durum Takip Tablosu

| Modül | Tasarım | Implementasyon | Test | Entegrasyon |
|-------|---------|----------------|------|-------------|
| ML Core (nn, optim, ensemble) | ✅ | 🔄 | 🔄 | ⬜ |
| GPU Kernels (CUDA/Metal) | ✅ | 🔄 | ⬜ | ⬜ |
| AutoML Trainer | ✅ | 🔄 | ⬜ | ⬜ |
| AI Stdlib (296 dosya) | ✅ | 🔄 | ⬜ | ⬜ |
| Kernel Core (scheduler, IPC) | ✅ | ✅ | 🔄 | ⬜ |
| FFI Bridges (Swift, Kotlin, Rust...) | ✅ | ✅ | ✅ | 🔄 |
| Platform Targets (7 hedef) | ✅ | 🔄 | ⬜ | ⬜ |
| ZenFlow UI | ✅ | 🔄 | ⬜ | ⬜ |
| Web Framework | ✅ | 🔄 | ⬜ | ⬜ |
| Science Stdlib | ✅ | 🔄 | ⬜ | ⬜ |
| Compiler (self-hosting) | ✅ | 🔄 | 🔄 | ⬜ |
| Toolchain (znpkg, znfmt...) | ✅ | ✅ | 🔄 | 🔄 |
| Security (capability, sandbox) | ✅ | 🔄 | ⬜ | ⬜ |
| Formal Verification (CVC5/Isabelle) | ✅ | 🔄 | ⬜ | ⬜ |

> ✅ Tamamlandı &nbsp;|&nbsp; 🔄 Devam ediyor &nbsp;|&nbsp; ⬜ Henüz başlanmadı

---

## 📁 Dizin Yapısı

```
docs/ml/check/
├── INDEX.md                        ← Bu dosya (ana index)
├── ROADMAP_ML_AI.md                ← ML + AI (354 dosya)
├── ROADMAP_KERNEL_FFI_PLATFORM.md  ← Kernel + FFI + Platform (60 dosya)
├── ROADMAP_STDLIB.md               ← Standard Library (709 dosya)
├── ROADMAP_COMPILER_TOOLCHAIN.md   ← Compiler + Toolchain (359 dosya)
└── ROADMAP_SECURITY.md             ← Security + Formal Verification (34 dosya)
```

---

## 🚀 Öneri: Sonraki Adımlar

1. Her roadmap dosyasındaki `[ ]` checkbox'larını doldurmaya başla
2. `zn/ml/kernels/cuda/` ve `metal/` GPU kernel'lerini önceliklendir
3. FFI bridge'lerin ABI snapshot testlerini genişlet
4. ZenFlow UI framework'ü demo app ile test et
5. Formal verification pipeline'ını CVC5 ile otomatize et
