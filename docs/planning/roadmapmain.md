# Nova Lang — Proje Yapısı & Yol Haritası

> **Dil:** Nova v10.0 · Intent-First · Flow Types · Unit Algebra · Proof Terms  
> **Mimari:** C bootstrap çekirdeği + ZN self-hosting katmanı  
> **Durum:** Frontend ZN dosyaları hazır → C bootstrap yazılacak

---

## Katman Mimarisi

```
┌─────────────────────────────────────────────────────┐
│  KULLANICI KODU  (.zn dosyaları)                     │
├─────────────────────────────────────────────────────┤
│  nova/          ZN ile yazılmış üst katman           │
│  ├── src/compiler/    compiler (self-hosting)        │
│  ├── src/runtime/     async, memory, ffi, wasm       │
│  ├── src/packages/    std, nn, ui, cloud             │
│  ├── kernel/          ZN kernel servisleri           │
│  └── security/        capability, sandbox, crypto    │
├─────────────────────────────────────────────────────┤
│  core/          C ile yazılmış bootstrap çekirdeği   │
│  ├── src/compiler/    C lexer, parser, codegen       │
│  ├── src/backend/     bytecode, VM                   │
│  └── src/runtime/     GC, native bridge              │
├─────────────────────────────────────────────────────┤
│  kernel/        Bare-metal / OS katmanı              │
│  ├── arch/x86_64/     ASM boot, GDT, paging          │
│  └── arch/arm64/      AArch64 bootstrap              │
└─────────────────────────────────────────────────────┘
```

---

## Tam Dosya Yapısı & Öncelik Durumu

Semboller: `✅` hazır · `🔨` yazılacak · `📁` klasör (içi dolu olacak) · `⚠️` eksik tamamlanacak

---

### `/` Kök

```
nova/
├── README.md                   ✅
├── CODE_OF_CONDUCT.md          ✅
├── CONTRIBUTING.md             ✅
├── LICENSE                     ✅
├── build.zn                    🔨  P1 — build sistemi tanımı
├── zn.toml                     🔨  P1 — paket manifest
└── ...
```

---

### `core/` — C Bootstrap Compiler
>
> **FAZA 1 — Kritik Yol.** ZN dosyaları derlenebilmeden önce bu tamamlanmalı.

```
core/
├── Makefile                    🔨  P1 — ilk çalışan build
├── CMakeLists.txt              🔨  P2
├── meson.build                 🔨  P3
├── configure.ac                🔨  P3
├── compile_commands.json       (otomatik üretilir)
│
├── include/
│   ├── nova.h                  🔨  P1 — master include
│   ├── version.h               🔨  P1 — VERSION_MAJOR/MINOR/PATCH
│   ├── config.h.in             🔨  P2 — build config template
│   ├── compiler/               📁  her .c için .h buraya
│   │   ├── lexer.h             🔨  P1
│   │   ├── ast.h               🔨  P1
│   │   ├── parser.h            🔨  P1
│   │   ├── semantic.h          🔨  P1
│   │   └── codegen.h           🔨  P1
│   ├── runtime/
│   │   └── runtime.h           🔨  P2
│   ├── ai/                     📁  (ilerisi)
│   ├── kernel/                 📁  (ilerisi)
│   ├── platform/               📁  (ilerisi)
│   ├── security/               📁  (ilerisi)
│   └── std/                    📁  (ilerisi)
│
├── src/
│   ├── backend/
│   │   ├── opcode.h            🔨  P1 — YAZILACAK İLK DOSYA
│   │   ├── chunk.c             🔨  P1 — bytecode chunk + constant pool
│   │   ├── bytecode.c          🔨  P1 — encode/decode
│   │   └── vm.c                🔨  P1 — stack-based VM
│   │
│   ├── compiler/
│   │   ├── lexer.c             🔨  P1 — tokens.zn'i C'ye çevir
│   │   ├── ast.c               🔨  P1 — ast.zn node'larını C struct'a çevir
│   │   ├── parser.c            🔨  P1 — minimal parse_ fonksiyonları
│   │   ├── semantic.c          🔨  P1 — kapsam + tip kontrolü
│   │   ├── codegen.c           🔨  P1 — AST → bytecode
│   │   └── main.c              🔨  P1 — compiler entry point
│   │
│   ├── runtime/
│   │   └── runtime.c           🔨  P2 — GC, bellek, native call bridge
│   │
│   ├── ai/                     📁  (FAZA 9)
│   ├── kernel/                 📁  (FAZA 5)
│   ├── security/               📁  (FAZA 6)
│   └── std/                    📁  (FAZA 4)
│
├── tests/
│   ├── unit/                   🔨  P2 — her C modülü için
│   ├── integration/            🔨  P2 — pipeline testleri
│   ├── fuzz/                   🔨  P3 — lexer/parser fuzz
│   └── benchmarks/             🔨  P3 — VM perf
│
├── third_party/
│   ├── unity/                  🔨  P1 — test framework (önce)
│   ├── mimalloc/               🔨  P1 — hızlı allocator (önce)
│   ├── blake3/                 🔨  P2 — hash/crypto
│   └── llvm/                   🔨  P4 — native codegen (en son)
│
└── tools/
    ├── znc/                    🔨  P2 — C compiler driver
    ├── zndbg/                  🔨  P3 — debugger
    ├── znfmt/                  🔨  P3 — formatter (C impl)
    ├── znlsp/                  🔨  P4 — Language Server
    └── znrepl/                 🔨  P3 — REPL (C impl)
```

**C Bootstrap build sırası:**

```
opcode.h → chunk.c → bytecode.c → lexer.c → ast.c → parser.c → semantic.c → codegen.c → vm.c → main.c
```

---

### `nova/` — ZN Self-Hosting Katmanı
>
> **FAZA 2+** — C bootstrap tamamlandıktan sonra.

```
nova/
├── main.zn                     🔨  P2 — nova entry point
├── lib.zn                      🔨  P2 — library root
│
├── src/compiler/
│   ├── mod.zn                  ✅  (var)
│   │
│   ├── frontend/               ← MEVCUT ZN DOSYALARIN BURAYA TAŞINACAK
│   │   ├── token.zn            ✅  hazır (taşı)
│   │   ├── ast.zn              ✅  hazır (taşı)
│   │   ├── lexer.zn            ✅  hazır (taşı)
│   │   ├── parser.zn           ✅  hazır (taşı)
│   │   ├── errors.zn           ✅  hazır (taşı)
│   │   ├── symbol_table.zn     ✅  hazır (taşı)
│   │   ├── semantic_analyzer.zn ✅  hazır (taşı)
│   │   ├── type_checker.zn     ⚠️  hazır, 1 placeholder kapatılacak
│   │   ├── module_resolver.zn  ✅  hazır (taşı)
│   │   └── ir_generator.zn     ⚠️  hazır, 1 TODO kapatılacak
│   │
│   ├── typesystem/             🔨  P2 — tip sistemi detayları
│   │   ├── types.zn            🔨  temel tip tanımları
│   │   ├── unification.zn      🔨  tip birleştirme
│   │   ├── inference.zn        🔨  Hindley-Milner
│   │   └── unit_algebra.zn     🔨  m/s² cebiri runtime
│   │
│   ├── ir/
│   │   ├── mod.zn              ✅  (var) — IRModule tanımı
│   │   ├── builder.zn          🔨  P2 — IR construction API
│   │   ├── ssa.zn              🔨  P2 — SSA form, Phi node'ları
│   │   └── passes.zn           🔨  P3 — IR dönüşüm geçişleri
│   │
│   ├── optimizer/              🔨  P3
│   │   ├── dead_code.zn
│   │   ├── inlining.zn
│   │   ├── unit_fold.zn        🔨  birim sabit katlama
│   │   └── tensor_opt.zn       🔨  tensor op füzyonu
│   │
│   └── backend/                🔨  P4
│       ├── llvm_backend.zn
│       ├── wasm_backend.zn
│       └── bytecode_backend.zn
│
├── src/runtime/
│   ├── memory_manager.zn       ✅  (var)
│   ├── task_scheduler.zn       ✅  (var)
│   ├── async_runtime.zn        ✅  (var)
│   ├── ffi_runtime.zn          ✅  (var)
│   ├── wasm_runtime.zn         ✅  (var)
│   └── reflection_runtime.zn   ✅  (var)
│
├── src/packages/
│   ├── nova_std/               🔨  P4 — std wrapper'ları
│   ├── nova_nn/                🔨  P6 — neural network
│   ├── nova_ui/                🔨  P6 — Aurora UI framework
│   ├── nova_cloud/             🔨  P6 — cloud primitives
│   └── framework/              🔨  P6 — web/app framework
│
├── src/tools/
│   ├── ai/                     🔨  P5
│   ├── formal/                 🔨  P5 — proof tools
│   ├── advanced/               🔨  P5
│   └── infra/                  🔨  P4
│
├── kernel/
│   ├── core/                   🔨  P5 — ZN kernel servisleri
│   ├── hal/                    🔨  P5 — Hardware Abstraction Layer
│   │   ├── cpu.zn
│   │   ├── timer.zn
│   │   └── uart.zn
│   ├── memory/                 🔨  P5
│   │   ├── allocator.zn
│   │   ├── paging.zn
│   │   └── heap.zn
│   ├── drivers/                🔨  P5
│   └── security/               🔨  P6
│
├── platform/
│   ├── targets/
│   │   ├── linux_x86_64.zn     ✅  (var)
│   │   ├── macos_arm64.zn      ✅  (var)
│   │   └── baremetal_x86_64.zn ✅  (var)
│   └── capabilities/           🔨  P5 — platform capability manifest
│       ├── linux.zn
│       ├── macos.zn
│       └── baremetal.zn
│
├── security/
│   ├── capability_model.zn     ✅  (var)
│   ├── sandboxing.zn           ✅  (var)
│   ├── crypto_primitives.zn    ✅  (var)
│   └── audit_log.zn            ✅  (var)
│
└── toolchain/
    ├── znfmt.zn                ✅  (var)
    ├── znlint.zn               ✅  (var)
    ├── zntest.zn               ✅  (var)
    ├── znrepl.zn               ✅  (var)
    ├── znpkg.zn                ✅  (var)
    ├── zndoc.zn                ✅  (var)
    └── znup.zn                 ✅  (var)
```

---

### `kernel/` — Bare-metal OS Katmanı
>
> **FAZA 5** — Runtime hazır olduktan sonra.

```
kernel/
├── build.zn                    🔨  P5 — kernel build tanımı
│
├── arch/
│   ├── x86_64/
│   │   ├── boot.asm            🔨  P5 — YAZILACAK İLK ASM — bootloader
│   │   ├── entry.c             🔨  P5 — kernel C entry point
│   │   ├── gdt.c               🔨  P5 — Global Descriptor Table
│   │   ├── interrupts.c        🔨  P5 — IDT, interrupt handler'lar
│   │   └── paging.c            🔨  P5 — sayfa tabloları, virtual memory
│   └── arm64/
│       ├── boot.asm            🔨  P5 — AArch64 bootstrap
│       ├── entry.c             🔨  P5
│       ├── interrupts.c        🔨  P5
│       └── paging.c            🔨  P5
│
├── core/
│   ├── scheduler.zn            ✅  (var)
│   ├── ipc.zn                  ✅  (var)
│   ├── memory.zn               🔨  P5 — virtual memory manager
│   ├── syscall.zn              🔨  P5 — syscall tablosu + dispatch
│   └── process.zn              🔨  P5 — process model, PCB
│
├── security/
│   ├── access_control.zn       ✅  (var)
│   └── capability_table.zn     ✅  (var)
│
├── configs/                    🔨  P5 — kernel config dosyaları
└── linker_scripts/             🔨  P5 — ld script'leri
```

---

### `std/` — Standart Kütüphane
>
> **FAZA 4** — Compiler self-hosting tamamlandıktan sonra.

```
std/
├── mod.zn                      🔨  P4 — root, prelude re-export
├── prelude.zn                  🔨  P4 — otomatik import edilecekler
├── option.zn                   🔨  P4 — Option<T>
├── result.zn                   🔨  P4 — Result<T,E>
├── error.zn                    🔨  P4 — Error trait
├── collections/                🔨  P4 — Vec, HashMap, HashSet, BTreeMap
├── string/                     🔨  P4 — String, &str, unicode
├── io/                         🔨  P4 — dosya, stdin/stdout
├── math/                       🔨  P4 — matematik, sabitler
├── concurrency/                🔨  P4 — Mutex, Channel, Arc
├── net/                        🔨  P5 — TCP/UDP/HTTP
├── crypto/                     🔨  P5 — blake3, AES
└── time/                       🔨  P4 — Duration, Instant, DateTime
```

---

### `docs/` — Dokümantasyon

```
docs/
├── architecture/
│   ├── OVERVIEW.md             🔨  güncelle
│   ├── DESIGN_PRINCIPLES.md    🔨  güncelle — Intent-First felsefesi
│   ├── ROADMAP.md              🔨  bu dosyayı referans al
│   └── ADR/
│       ├── 001-language-design.md   🔨  token/keyword kararları
│       ├── 002-runtime-model.md     🔨  memory modeli
│       └── 003-security-model.md    🔨  capability sistemi
├── framework/
│   └── README.md               🔨
├── kernel/
│   ├── MICROKERNEL_DESIGN.md   🔨
│   ├── MEMORY_MODEL.md         🔨
│   └── IPC_PROTOCOL.md         🔨
├── ml/
│   ├── README_ML.md            🔨
│   ├── NOVA_ML_SUMMARY.md      🔨
│   ├── ML_AI_IMPROVEMENTS.md   🔨
│   └── QUICKSTART_ML.md        🔨
└── security/
    ├── CAPABILITY_SYSTEM.md    🔨
    ├── SANDBOX_MODEL.md        🔨
    └── THREAT_MODEL.md         🔨
```

---

### `examples/` — Örnek Programlar

```
examples/
├── hello_world.zn              🔨  P1 — bootstrap test programı
├── formal_proof_demo.zn        🔨  P3 — require/ensure/proof
├── ai_inference_demo.zn        🔨  P6
├── microkernel_demo.zn         🔨  P5
├── mobile_app_demo.zn          🔨  P6
└── web_app_demo.zn             🔨  P6
```

---

### `tests/`, `scripts/`, `toolchain/`, `packages/`

```
tests/
├── test_runner.zn              ✅  (var)
├── unit/                       🔨  P2+
├── integration/                🔨  P2+
├── cross_platform/             🔨  P4+
└── benchmarks/                 🔨  P3+

scripts/
├── bootstrap.sh                🔨  P1 — C bootstrap derler
├── build.sh                    🔨  P1 — tam build
└── test.sh                     🔨  P2

toolchain/
├── ci/                         🔨  P3
├── installer/                  🔨  P4
└── release/                    🔨  P4

packages/
├── registry/                   🔨  P5
└── templates/                  🔨  P4
```

---

## Faz Özeti

| Faz | Kapsam | Kritik Dosyalar |
|-----|--------|-----------------|
| **F0** | Mimari kararlar | `docs/architecture/ADR/*.md` |
| **F1** | C bootstrap compiler | `core/src/backend/opcode.h` → `vm.c` → `main.c` |
| **F2** | ZN frontend self-hosting | 10 mevcut ZN dosyası taşınır + derlenir |
| **F3** | ZN runtime | `nova/src/runtime/*.zn` (hepsi var) |
| **F4** | std kütüphanesi | `std/mod.zn` → `prelude.zn` → `option/result/...` |
| **F5** | Kernel | `kernel/arch/x86_64/boot.asm` → kernel core |
| **F6** | Security | `nova/security/*.zn` (hepsi var, wire edilecek) |
| **F7** | Platform hedefleri | `nova/platform/targets/*.zn` (var, test edilecek) |
| **F8** | Araç zinciri | `nova/toolchain/*.zn` (var, derlenecek) |
| **F9** | Paketler | `nova/src/packages/nova_std/` → diğerleri |
| **F10** | Testler | Her fazla paralel |

---

## Şu An — Sonraki Somut Adım

```
1. core/src/backend/opcode.h      ← BURADAN BAŞLA
2. core/src/backend/chunk.c
3. core/src/backend/bytecode.c
4. core/src/compiler/lexer.c      ← tokens.zn'i C'ye çevir
5. core/src/compiler/ast.c        ← ast.zn node'larını C struct'a çevir
6. core/src/compiler/parser.c     ← minimal subset
7. core/src/compiler/semantic.c
8. core/src/compiler/codegen.c
9. core/src/backend/vm.c
10. core/src/compiler/main.c
    │
    ↓  znc binary hazır
    │
11. examples/hello_world.zn derle → başarılıysa bootstrap tamamdır
    │
    ↓
12. nova/src/compiler/frontend/ altına 10 ZN dosyayı taşı
13. type_checker.zn — 1 placeholder kapat
14. ir_generator.zn — 1 TODO kapat
15. ZN compiler kendini derler → self-hosting tamamdır
```
