# Compute Folder Organization Plan

## Current Files (21 files)

### 1️⃣ **Scheduling & Execution** → `scheduling/`
- nova_scheduler.c
- nova_cognitive_scheduler.c
- nova_dispatcher.c
- nova_execution_fabric.c

### 2️⃣ **JIT & Compilation** → `jit/`
- nova_jit.c
- nova_jit_attest.c

### 3️⃣ **Cloud Providers & Bridges** → `providers/` (already exists!)
- nova_bridge.c
- nova_gemini_bridge.c
- nova_providers_bridge.c

### 4️⃣ **Security & Crypto** → `security/`
- nova_crypto.c
- nova_shield.c

### 5️⃣ **Compute Economics & Optimization** → `optimization/`
- nova_compute_economics.c
- nova_shard.c
- nova_context.c

### 6️⃣ **Core Compute** → `core/`
- nova_compute.c

### 7️⃣ **Mirror & ML** → `mirror/`
- nova_mirror.c
- nova_mirror_v2.c
- nova_mirror_ml.c

### 8️⃣ **Legacy & Tests** → `legacy/` and `tests/`
- nova_kernel_contracts_legacy.c → legacy/
- gemini_nova_benchmark.c → tests/
- simple_matmul_test.c → tests/

## New Structure:
```
compute/
├── core/
│   └── nova_compute.c
├── scheduling/
│   ├── nova_scheduler.c
│   ├── nova_cognitive_scheduler.c
│   ├── nova_dispatcher.c
│   └── nova_execution_fabric.c
├── jit/
│   ├── nova_jit.c
│   └── nova_jit_attest.c
├── providers/
│   ├── nova_bridge.c
│   ├── nova_gemini_bridge.c
│   └── nova_providers_bridge.c
├── security/
│   ├── nova_crypto.c
│   └── nova_shield.c
├── optimization/
│   ├── nova_compute_economics.c
│   ├── nova_shard.c
│   └── nova_context.c
├── mirror/
│   ├── nova_mirror.c
│   ├── nova_mirror_v2.c
│   └── nova_mirror_ml.c
├── legacy/
│   └── nova_kernel_contracts_legacy.c
└── tests/
    ├── gemini_nova_benchmark.c
    └── simple_matmul_test.c
```
