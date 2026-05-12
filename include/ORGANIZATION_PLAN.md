# Include Headers Organization Plan

## Analysis: ~130+ header files!

### Categories:

### 1️⃣ **Compiler** → `compiler/`
- nova_ast.h, nova_parser.h, nova_lexer.h, nova_semantic.h
- nova_codegen.h, nova_types.h, nova_ir.h
- nova_borrow_checker.h, nova_pattern.h, nova_generics.h

### 2️⃣ **Backend** → `backend/`
- nova_backend_*.h, nova_cpu_backend.h
- nova_jit.h, nova_llvm_*.h

### 3️⃣ **Memory** → `memory/`
- nova_gc.h, nova_gc_concurrent.h, nova_allocator.h
- nova_arena.h, nova_memory.h, nova_memory_arena.h

### 4️⃣ **Runtime** → `runtime/`
- nova_runtime_*.h, nova_execution_fabric.h
- nova_scheduler.h, nova_dispatcher.h, nova_events.h

### 5️⃣ **ML/AI** → `ml/`
- nova_nn.h, nova_tensor.h, nova_tensor_ops.h
- nova_loss.h, nova_optimizer.h, nova_training.h
- nova_mirror*.h, nova_gpt_backend.h

### 6️⃣ **Compute** → `compute/`
- nova_compute.h, nova_kernels.h, nova_autotune.h
- nova_cluster.h, nova_distributed.h, nova_shard.h

### 7️⃣ **Formal** → `formal/`
- nova_proof.h, nova_formal.h, nova_solver.h
- nova_invariants.h, nova_obligation.h

### 8️⃣ **System** → `system/`
- nova_common.h, nova_types.h, nova_error.h
- nova_span.h, nova_builtins.h, nova_limits.h

### 9️⃣ **Security** → `security/`
- nova_crypto.h, nova_attest.h

### 🔟 **Tools** → `tools/`
- nova_profiler_v2.h, nova_monitor.h, nova_health.h

## Action: Create subdirectories and move headers
