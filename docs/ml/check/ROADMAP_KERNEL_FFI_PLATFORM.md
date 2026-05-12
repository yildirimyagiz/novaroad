# 🚀 Nova Kernel / FFI / Platform — Feature Roadmap

**Last Updated:** 2024  
**Language:** Nova (zn)  
**Status:** Active Development  

---

## 📊 Overview — Modül Özeti

| Module | Files | Subsystems | Status | Priority |
|--------|-------|-----------|--------|----------|
| 🔧 **Kernel** | 36 | core, drivers, hal, memory, security | In Progress | Critical |
| 🌉 **FFI** | 13 | C/JS/Py/Rust/Swift/Kotlin bridges | In Progress | High |
| 🖥️ **Platform** | 11 | 7 targets + 3 capabilities | In Progress | High |
| **TOTAL** | **60** | **13 subsystems** | 📈 Growing | ✅ Ready |

---

## 🔧 KERNEL MODULE (zn/kernel/)

### 📋 Modül Açıklaması
Nova Kernel subsystemi, işletim sistemi seviyesinde görevler ve kaynakları yönetir. Boot sistemi, IPC mekanizmaları, dinamik modül yükleme, hata işleme, görev planlayıcı, sistem çağrıları, donanım soyutlama katmanı (HAL), bellek yönetimi ve güvenlik özellikleri içerir.

### 📁 Kernel Subsystem Breakdown

#### ⚙️ CORE SUBSYSTEM (12 files)
**Açıklama:** Çekirdek sistem bileşenleri ve kontratlar

**Dosya Listesi:**

| File | Purpose | Status |
|------|---------|--------|
| `boot.zn` | Boot system entry point | ✅ Design |
| `boot_contracts.zn` | Boot configuration contracts | 🔒 FROZEN |
| `ipc.zn` | Inter-process communication | ✅ Design |
| `ipc_contracts.zn` | IPC channel contracts | 🔒 FROZEN |
| `mod.zn` | Dynamic module loading | ✅ Design |
| `mod_contracts.zn` | Module dependency contracts | ✅ Design |
| `panic.zn` | Panic unwinding system | ✅ Design |
| `panic_contracts.zn` | Panic contracts | ✅ Design |
| `scheduler.zn` | M:N task scheduler | ✅ Design |
| `scheduler_contracts.zn` | Scheduler contracts | ✅ Design |
| `syscall.zn` | System call interface | ✅ Design |
| `syscall_contracts.zn` | Syscall contracts | ✅ Design |

**Özellikler / Features:**

- 🥾 **Boot System**: BootConfig, BootState
  - `thread_pool_size` — İş parçacığı havuzu boyutu
  - `max_tasks` — Maksimum görev sayısı
  - `gc_threshold` — Çöp toplama eşiği
  - `enable_metrics` — Metrikleri etkinleştir

- 📡 **IPC (FROZEN API)**: 
  - `ChannelId` — Kanal tanımlayıcı
  - `Sender<T>`, `Receiver<T>` — Tip güvenli kanal uçları
  - Async/await desteği
  - Timeout yönetimi

- 📦 **Dynamic Module Loading**:
  - `ModuleId`, `ModuleHandle`, `Version`
  - Bağımlılık çözünürlüğü
  - Hot reload desteği
  - ABI versioning

- 😱 **Panic Handling**:
  - setjmp/longjmp unwinding
  - `PanicInfo` yapısı (message, file, line, col)
  - Stack trace capturing
  - Custom panic handlers

- 🎯 **M:N Task Scheduler**:
  - Cooperative scheduling
  - `TaskState` enumi: Ready, Running, Blocked, Completed
  - Work-stealing queue
  - Priority levels

- 🔌 **System Calls**:
  - Kernel interface layer
  - Cross-platform abstraction

**Status Kontrol Listesi:**
- [ ] Tasarım (Design)
- [ ] Implementasyon (Implementation)
- [ ] Unit Test (Testing)
- [ ] Entegrasyon (Integration)

---

#### 🖥️ DRIVERS SUBSYSTEM (7 files)
**Açıklama:** Donanım sürücü arayüzleri

**Dosya Listesi:**
| File | Device Type | Status |
|------|-------------|--------|
| `audio.zn` | Audio hardware | ✅ Design |
| `display.zn` | Display/Video | ✅ Design |
| `gpu.zn` | Graphics processing | ✅ Design |
| `input.zn` | Input devices | ✅ Design |
| `network.zn` | Network interfaces | ✅ Design |
| `storage.zn` | Storage devices | ✅ Design |
| `mod.zn` | Driver module root | ✅ Design |

**Özellikler / Features:**
- 🔊 Audio: PCM, DSD, codec support
- 🎮 Input: Keyboard, mouse, joystick, touch
- 🌐 Network: TCP/IP, UDP, packet filtering
- 💾 Storage: Block devices, filesystems
- 🎬 Display: Framebuffer, scanout, vsync
- 🎨 GPU: Shaders, rendering pipeline

**Status Kontrol Listesi:**
- [ ] Tasarım (Design)
- [ ] Implementasyon (Implementation)
- [ ] Unit Test (Testing)
- [ ] Entegrasyon (Integration)

---

#### 🏗️ HAL SUBSYSTEM (5 files) — Hardware Abstraction Layer
**Açıklama:** Platform-spesifik donanım ayrıntılarının soyutlanması

**Dosya Listesi:**
| Platform | File | Target | Status |
|----------|------|--------|--------|
| ARM64 | `aarch64.zn` | Apple Silicon, Android ARM64 | ✅ Design |
| x86-64 | `x86_64.zn` | Intel/AMD desktop/server | ✅ Design |
| RISC-V | `riscv.zn` | Open ISA boards | ✅ Design |
| WebAssembly | `wasm.zn` | Browser/edge runtimes | ✅ Design |
| Module | `mod.zn` | HAL root | ✅ Design |

**Özellikler / Features:**
- 📜 ISA-specific intrinsics
- ⚡ Atomic operations
- 🔄 Cache management
- 🛡️ TLB shootdown
- 🧵 Memory barriers

**Status Kontrol Listesi:**
- [ ] Tasarım (Design)
- [ ] Implementasyon (Implementation)
- [ ] Unit Test (Testing)
- [ ] Entegrasyon (Integration)

---

#### 🧠 MEMORY SUBSYSTEM (6 files)
**Açıklama:** Bellek yönetimi, tahsisi ve sayfalama

**Dosya Listesi:**
| File | Purpose | Status |
|------|---------|--------|
| `allocator.zn` | Memory allocator | ✅ Design |
| `dma.zn` | Direct Memory Access | ✅ Design |
| `heap.zn` | Heap management | ✅ Design |
| `paging.zn` | Virtual memory paging | ✅ Design |
| `vmm.zn` | Virtual Memory Manager | ✅ Design |
| `mod.zn` | Memory module root | ✅ Design |

**Özellikler / Features:**
- 🎯 Arena allocators
- 🔐 DMA buffer management
- 🗑️ Garbage collection
- 📄 Page table management
- 🔒 Memory protection
- 💨 Lazy page faults

**Status Kontrol Listesi:**
- [ ] Tasarım (Design)
- [ ] Implementasyon (Implementation)
- [ ] Unit Test (Testing)
- [ ] Entegrasyon (Integration)

---

#### 🔐 SECURITY SUBSYSTEM (5 files)
**Açıklama:** Güvenlik, kriptografi ve sandbox mekanizmaları

**Dosya Listesi:**
| File | Purpose | Status |
|------|---------|--------|
| `capabilities.zn` | Capability-based security | ✅ Design |
| `crypto.zn` | Cryptographic primitives | ✅ Design |
| `sandbox.zn` | Process sandboxing | ✅ Design |
| `tee.zn` | Trusted Execution Environment | ✅ Design |
| `mod.zn` | Security module root | ✅ Design |

**Özellikler / Features:**
- 🔑 Capability tokens
- 🔒 AES, SHA, HMAC
- 🛡️ Seccomp filters
- 🏠 TEE bridges (TrustZone, SGX)
- 🚫 ASLR, DEP support
- 🔐 Privilege separation

**Status Kontrol Listesi:**
- [ ] Tasarım (Design)
- [ ] Implementasyon (Implementation)
- [ ] Unit Test (Testing)
- [ ] Entegrasyon (Integration)

---

## 🌉 FFI MODULE (zn/ffi/)

### 📋 Modül Açıklaması
Nova FFI subsystemi, dış dile arabirimi (Foreign Function Interface) yönetir. C ABI kontratları, C/JavaScript/Python/Rust/Swift/Kotlin köprüleri, ve ABI kararlılık testleri içerir. **FROZEN C ABI kontratları** hassas interoperabilite garantileri sağlar.

### 📁 FFI Files & Bridges

**Dosya Listesi:**

| File | Bridge Type | Status | Tested |
|------|-------------|--------|--------|
| `mod.zn` | C ABI Root (FROZEN) | 🔒 FROZEN | ✅ Yes |
| `c_bridge.zn` | C language | ✅ Design | ✅ Yes |
| `enhanced_ffi.zn` | Enhanced features | ✅ Design | ⏳ Partial |
| `interop.zn` | General interop | ✅ Design | ✅ Yes |
| `native.zn` | Native platform | ✅ Design | ✅ Yes |
| `js_bridge.zn` | JavaScript (WASM) | ✅ Design | ✅ Yes |
| `py_bridge.zn` | Python | ✅ Design | ⏳ In Progress |
| `rust_bridge.zn` | Rust | ✅ Design | ✅ Yes |
| `swift_bridge.zn` | Swift/iOS | ✅ Design | ✅ Smoke Tested |
| `swift_smoke_test.zn` | Swift validation | ✅ Design | ✅ Passed |
| `kotlin_smoke_test.zn` | Kotlin/Android validation | ✅ Design | ✅ Passed |
| `abi_snapshot_tests.zn` | ABI stability | ✅ Design | ✅ Yes |
| `examples.zn` | Example code | ✅ Design | ✅ Yes |

**🔒 FROZEN C ABI Kontratları (mod.zn):**

```
Extern C Naming Convention:
  nova_[module]_[function]
  
String Ownership Rules:
  - Input strings: Copy-in (caller frees)
  - Output strings: Caller frees
  
Panic Behavior:
  ⚠️ Panic NEVER crosses FFI boundary
  → Use Result<T, Error> for error propagation
  
Struct Layout:
  - C-compatible memory layout
  - No padding assumptions
  - Explicit alignment (#[repr(C)])
```

### 🌉 Bridge Capabilities

**C Bridge (c_bridge.zn):**
- ✅ Direct C function calls
- ✅ Struct marshaling
- ✅ Opaque pointers
- ✅ Callback support

**JavaScript Bridge (js_bridge.zn):**
- ✅ WASM module loading
- ✅ JavaScript interop
- ✅ Promise/async support
- ✅ WebAssembly memory sharing

**Python Bridge (py_bridge.zn):**
- ✅ CPython C API
- ✅ Object marshaling
- ✅ GIL handling
- ✅ Type conversion

**Rust Bridge (rust_bridge.zn):**
- ✅ FFI-safe types
- ✅ Error handling
- ✅ Lifetime management
- ✅ Generic support

**Swift Bridge (swift_bridge.zn):**
- ✅ Objective-C interop
- ✅ String roundtrip ✅ TESTED
- ✅ Result<T> handling ✅ TESTED
- ✅ Memory management
- 🧪 Smoke Test: `swift_smoke_test.zn`

**Kotlin Bridge (kotlin_smoke_test.zn):**
- ✅ JNI integration
- ✅ String roundtrip via JNI ✅ TESTED
- ✅ Array marshaling
- ✅ Exception handling

### 📊 ABI Stability Testing (abi_snapshot_tests.zn)

**Test Coverage:**
- ✅ Struct layouts
- ✅ Function signatures
- ✅ Calling conventions
- ✅ Type size verification
- ✅ Alignment guarantees

**Status Kontrol Listesi:**
- [x] Tasarım (Design) — Tamamlandı
- [x] Implementasyon (Implementation) — Tamamlandı
- [x] Unit Test (Testing) — Tamamlandı
- [ ] Entegrasyon (Integration) — In Progress

---

## 🖥️ PLATFORM MODULE (zn/platform/)

### 📋 Modül Açıklaması
Nova Platform subsystemi, 7 hedef platform için kod üretim ve yetenekleri yönetir. macOS (Intel/Apple Silicon), iOS, Android, Linux, Windows ve WebAssembly desteklenir.

### 📁 Platform Targets (7 files)

**Açıklama:** Platform-spesifik kod ve yapılandırma

| Platform | File | Status | Tested | Priority |
|----------|------|--------|--------|----------|
| 🍎 Apple Silicon macOS | `aarch64_macos.zn` | ✅ Design | ✅ Yes | Critical |
| 🍎 Intel macOS | `x86_64_macos.zn` | ✅ Design | ✅ Yes | Critical |
| 📱 iOS/iPadOS | `aarch64_ios.zn` | ✅ Design | ✅ Yes | High |
| 🤖 Android ARM64 | `aarch64_android.zn` | ✅ Design | ✅ Yes | High |
| 🐧 Linux x86_64 | `x86_64_linux.zn` | ✅ Design | ✅ Yes | High |
| 🪟 Windows x86_64 | `x86_64_windows.zn` | ✅ Design | ⏳ In Progress | High |
| 🌐 Browser WebAssembly | `wasm32_browser.zn` | ✅ Design | ✅ Yes | Medium |

### 🎯 Platform Capabilities (3 files)

**Dosya Listesi:**

| File | Capability | Status | Platforms |
|------|-----------|--------|-----------|
| `gpu.zn` | GPU detection | ✅ Design | macOS, iOS, Android, Linux, Windows |
| `neural_engine.zn` | Apple Neural Engine | ✅ Design | macOS (M1+), iOS (A12+) |
| `simd.zn` | SIMD instruction sets | ✅ Design | All (platform-specific) |
| `mod.zn` | Platform module root | ✅ Design | All |

**🎮 GPU Capabilities (gpu.zn):**
- 🎬 Metal (macOS/iOS)
- 🎬 Vulkan (Android/Linux)
- 🎬 Direct3D (Windows)
- 🎬 WebGL (Browser)
- 📊 VRAM detection
- 🔍 Feature detection (ray tracing, compute)

**🧠 Neural Engine (neural_engine.zn):**
- 🤖 Apple Neural Engine (ANE)
- 📊 Model inference
- ⚡ Hardware acceleration
- 🎯 16-core NPU support
- 📈 Performance monitoring

**⚡ SIMD Instruction Sets (simd.zn):**
- 📦 NEON (ARM64)
- 📦 AVX/AVX-512 (x86-64)
- 📦 SSE (x86)
- 📦 RISC-V Vector
- 📦 WASM SIMD

**Status Kontrol Listesi:**
- [ ] Tasarım (Design)
- [ ] Implementasyon (Implementation)
- [ ] Unit Test (Testing)
- [ ] Entegrasyon (Integration)

---

## 🔀 Cross-Platform Compatibility Matrix

### ✅ Supported Platform Combinations

| Feature | macOS Intel | macOS Apple | iOS | Android | Linux | Windows | WASM |
|---------|:-----------:|:-----------:|:---:|:-------:|:-----:|:-------:|:----:|
| **Kernel Core** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ⚠️ |
| **IPC** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ⚠️ |
| **Scheduler** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ⚠️ |
| **HAL** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Memory Mgmt** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Security** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ⚠️ |
| **C FFI** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **JS Bridge** | ✅ | ✅ | ⚠️ | ⚠️ | ✅ | ✅ | ✅ |
| **Swift Bridge** | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| **Kotlin Bridge** | ❌ | ❌ | ❌ | ✅ | ❌ | ❌ | ❌ |
| **Python Bridge** | ✅ | ✅ | ⚠️ | ⚠️ | ✅ | ✅ | ⚠️ |
| **Rust Bridge** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **GPU Support** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Neural Engine** | ❌ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| **SIMD** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

**Legend:**
- ✅ Fully Supported
- ⚠️ Partially Supported (with limitations)
- ❌ Not Supported

---

## 📈 Implementation Progress & Roadmap

### Phase 1: Core Kernel (Weeks 1-8)
- [x] Boot system design
- [x] IPC foundation (FROZEN API)
- [ ] Module loading implementation
- [ ] Panic handling stack
- [ ] M:N scheduler prototype
- [ ] Syscall layer

### Phase 2: HAL & Drivers (Weeks 9-16)
- [ ] aarch64 HAL implementation
- [ ] x86_64 HAL implementation
- [ ] RISC-V HAL baseline
- [ ] WASM HAL support
- [ ] Audio driver interface
- [ ] Display driver interface
- [ ] Network driver interface
- [ ] Storage driver interface

### Phase 3: Memory & Security (Weeks 17-24)
- [ ] Allocator implementation
- [ ] DMA management
- [ ] Paging system
- [ ] Virtual memory manager
- [ ] Capability-based security
- [ ] Cryptographic primitives
- [ ] Sandbox mechanisms
- [ ] TEE integration

### Phase 4: FFI Implementation (Weeks 25-32)
- [x] C bridge (FROZEN)
- [x] C ABI contracts (FROZEN)
- [ ] Enhanced FFI features
- [x] Rust bridge
- [x] JavaScript bridge (WASM)
- [ ] Python bridge completion
- [x] Swift bridge + smoke tests
- [x] Kotlin bridge + smoke tests
- [x] ABI snapshot tests

### Phase 5: Platform Support (Weeks 33-40)
- [x] macOS (Intel) support
- [x] macOS (Apple Silicon) support
- [x] iOS support
- [x] Android support
- [ ] Linux (x86_64) support
- [ ] Windows (x86_64) support
- [x] WebAssembly support
- [x] GPU capability detection
- [x] Apple Neural Engine support
- [x] SIMD instruction mapping

### Phase 6: Integration & Testing (Weeks 41-48)
- [ ] End-to-end kernel tests
- [ ] FFI integration tests
- [ ] Cross-platform CI/CD
- [ ] Performance benchmarks
- [ ] Security audits
- [ ] Memory leak analysis
- [ ] Documentation completion

---

## 🎯 Key Milestones

| Milestone | Target Date | Status | Deliverables |
|-----------|------------|--------|--------------|
| **Kernel MVP** | Q2 2024 | 🔄 In Progress | Boot, IPC (frozen), scheduler |
| **FFI Ready** | Q2 2024 | ✅ Mostly Ready | C, Rust, JS, Swift, Kotlin bridges |
| **Platform 7/7** | Q3 2024 | 🔄 In Progress | All target platforms |
| **Security Hardened** | Q3 2024 | 📋 Planned | Sandbox, crypto, TEE |
| **GA Release** | Q4 2024 | 📋 Planned | Production-ready |

---

## 📝 Contract Pairs Reference

**Pattern:** `component.zn` + `component_contracts.zn`

### Kernel Core Contracts

```
boot.zn ←→ boot_contracts.zn
  BootConfig { thread_pool_size, max_tasks, gc_threshold, enable_metrics }
  BootState { ... }

ipc.zn ←→ ipc_contracts.zn (🔒 FROZEN)
  ChannelId, Sender<T>, Receiver<T>
  Guarantees: async-safe, panic-safe

mod.zn ←→ mod_contracts.zn
  ModuleId, ModuleHandle, Version
  Dependency resolution, hot reload

panic.zn ←→ panic_contracts.zn
  PanicInfo { message, file, line, col }
  setjmp/longjmp unwinding

scheduler.zn ←→ scheduler_contracts.zn
  TaskState { Ready, Running, Blocked, Completed }
  M:N scheduling guarantees

syscall.zn ←→ syscall_contracts.zn
  System call interface definitions
```

---

## 🔍 Testing Strategy

### Unit Testing
- ✅ Individual component tests
- ✅ Contract validation tests
- ✅ Edge case coverage
- ✅ Memory safety checks

### Integration Testing
- ✅ Cross-component interactions
- ✅ FFI boundary tests
- ✅ Platform-specific tests
- ⏳ End-to-end workflows

### ABI Testing
- ✅ Structure layout verification
- ✅ Calling convention tests
- ✅ Type size assertions
- ✅ Alignment guarantees
- ✅ Snapshot tests for stability

### Platform Testing
- ✅ macOS (both architectures)
- ✅ iOS
- ✅ Android
- ⏳ Linux
- ⏳ Windows
- ✅ WebAssembly

---

## 📊 Progress Tracker Dashboard

### Overall Completion: 45%

```
Kernel Core       ████████░░░░░░░░░░ 40%
FFI Bridges       █████████████░░░░░░ 70%
Platform Targets  ████████████░░░░░░░ 60%
HAL Layer         █████░░░░░░░░░░░░░░ 25%
Drivers           ███░░░░░░░░░░░░░░░░ 15%
Memory System     █████░░░░░░░░░░░░░░ 25%
Security         ████░░░░░░░░░░░░░░░░ 20%
```

### File Count Summary
- **Total Files:** 60
- **Designed:** 60 (100%)
- **Implemented:** 27 (~45%)
- **Tested:** 18 (~30%)
- **Production Ready:** 8 (~13%)

---

## 🚀 Next Actions

### Immediate Priority (Next 2 weeks)
1. ⚠️ Complete Linux (x86_64) platform support
2. ⚠️ Finalize Windows (x86_64) platform support
3. ⚠️ Implement Python bridge completion
4. ⚠️ Add Windows driver stubs

### Short Term (Next 4 weeks)
1. 📋 HAL layer full implementation
2. 📋 Driver interface implementation
3. 📋 Memory system hardening
4. 📋 Security features integration

### Medium Term (Next 8 weeks)
1. 🔄 Comprehensive integration testing
2. 🔄 Performance optimization
3. 🔄 Documentation updates
4. 🔄 Security audit

---

## 📚 Related Documentation

- [Kernel Architecture](./kernel/)
- [FFI Guidelines](./ffi/)
- [Platform Targets](./platform/)
- [API Reference](../api/)
- [Contributing Guide](../../CONTRIBUTING.md)

---

**Last Updated:** 2024-01-XX  
**Maintained By:** Nova Development Team  
**License:** Nova License v1.0
