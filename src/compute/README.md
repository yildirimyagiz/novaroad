# Nova Compute Module

Comprehensive compute infrastructure for Nova language.

## Structure

### 📦 `core/`
**Core compute functionality**
- `nova_compute.c` - Main compute engine

### ⏱️ `scheduling/`
**Task scheduling and execution**
- `nova_scheduler.c` - Task scheduler
- `nova_cognitive_scheduler.c` - AI-powered scheduling
- `nova_dispatcher.c` - Task dispatcher
- `nova_execution_fabric.c` - Execution framework

### ⚡ `jit/`
**Just-In-Time compilation**
- `nova_jit.c` - JIT compiler
- `nova_jit_attest.c` - JIT attestation/verification

### 🔒 `security/`
**Security and cryptography**
- `nova_crypto.c` - Cryptographic operations
- `nova_shield.c` - Security shield

### 📊 `optimization/`
**Performance optimization**
- `nova_compute_economics.c` - Cost optimization
- `nova_shard.c` - Data sharding
- `nova_context.c` - Execution context

### 🪞 `mirror/`
**Mirror ML systems**
- `nova_mirror.c` - Mirror system v1
- `nova_mirror_v2.c` - Mirror system v2
- `nova_mirror_ml.c` - ML-specific mirror

### 🌐 `providers/`
**Cloud provider integrations**
- `nova_bridge.c` - Provider bridge
- `nova_gemini_bridge.c` - Google Gemini integration
- `nova_providers_bridge.c` - Multi-provider bridge
- Cloud providers: AetherCloud, GhostMesh, NexusFoundry, NovaSilicon

### 🧪 `tests_compute/`
**Tests and benchmarks**
- `gemini_nova_benchmark.c` - Gemini benchmarks
- `simple_matmul_test.c` - Matrix multiplication tests

### 📚 `legacy/`
**Legacy code (deprecated)**
- `nova_kernel_contracts_legacy.c` - Old kernel contracts

## Usage

```c
#include "core/nova_compute.h"
#include "scheduling/nova_scheduler.h"
#include "jit/nova_jit.c"

// Initialize compute
nova_compute_init();

// Schedule tasks
nova_scheduler_schedule(task);

// JIT compile
nova_jit_compile(code);
```

## Organization Benefits

- ✅ **Modular** - Clear separation of concerns
- ✅ **Discoverable** - Easy to find relevant code
- ✅ **Maintainable** - Simple to update and extend
- ✅ **Scalable** - Easy to add new features

---

*Organized: March 2, 2026*
