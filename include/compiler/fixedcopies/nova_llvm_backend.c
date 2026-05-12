/**
 * Nova LLVM Backend - Full Implementation
 *
 * Provides LLVM-based JIT compilation and execution for Nova.
 */

#include "nova_llvm_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LLVM

/*
 * FIX #2: Single authoritative struct definition using the named tag
 * declared in the header's forward declaration.
 */
struct NovaLLVMContext {
  LLVMContextRef        context;
  LLVMModuleRef         module;
  LLVMBuilderRef        builder;
  LLVMExecutionEngineRef engine;
  LLVMPassManagerRef    mpm;
  char                 *target_triple;
  char                 *cpu_name;
  char                 *features;
  NovaOptLevel          opt_level;
  NovaTargetArch        target_arch;
  /*
   * FIX #1: Track whether the engine owns the module so destroy_context
   * never double-frees it.
   */
  bool engine_owns_module;
};

// ═══════════════════════════════════════════════════════════════════════════
// Accessor helpers (opaque struct — used by codegen.c)
// ═══════════════════════════════════════════════════════════════════════════

LLVMContextRef nova_llvm_get_llvm_context(NovaLLVMContext *ctx) {
  return ctx ? ctx->context : NULL;
}
LLVMModuleRef nova_llvm_get_module(NovaLLVMContext *ctx) {
  return ctx ? ctx->module : NULL;
}
LLVMBuilderRef nova_llvm_get_builder(NovaLLVMContext *ctx) {
  return ctx ? ctx->builder : NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// Context Management
// ═══════════════════════════════════════════════════════════════════════════

NovaLLVMContext *nova_llvm_create_context(const char *module_name) {
  NovaLLVMContext *ctx = (NovaLLVMContext *)calloc(1, sizeof(NovaLLVMContext));
  if (!ctx)
    return NULL;

  LLVMLinkInMCJIT();
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmParsers();
  LLVMInitializeAllAsmPrinters();
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  ctx->context = LLVMContextCreate();
  ctx->module  = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
  ctx->builder = LLVMCreateBuilderInContext(ctx->context);
  ctx->engine_owns_module = false;

  ctx->target_triple = LLVMGetDefaultTargetTriple();
  LLVMSetTarget(ctx->module, ctx->target_triple);

  ctx->cpu_name = LLVMGetHostCPUName();
  ctx->features = LLVMGetHostCPUFeatures();

  /* FIX #11: guard NULL returns on unusual platforms */
  if (!ctx->features) ctx->features = LLVMCreateMessage("");
  if (!ctx->cpu_name) ctx->cpu_name = LLVMCreateMessage("generic");

  return ctx;
}

void nova_llvm_destroy_context(NovaLLVMContext *ctx) {
  if (!ctx)
    return;

  if (ctx->mpm)
    LLVMDisposePassManager(ctx->mpm);

  if (ctx->engine) {
    /*
     * FIX #1: DisposeExecutionEngine also frees the module it owns.
     * Only dispose the module separately when no engine was created.
     */
    LLVMDisposeExecutionEngine(ctx->engine);
  } else if (ctx->module) {
    LLVMDisposeModule(ctx->module);
  }

  if (ctx->builder)
    LLVMDisposeBuilder(ctx->builder);
  if (ctx->context)
    LLVMContextDispose(ctx->context);

  LLVMDisposeMessage(ctx->target_triple);
  LLVMDisposeMessage(ctx->cpu_name);
  LLVMDisposeMessage(ctx->features);

  free(ctx);
}

// ═══════════════════════════════════════════════════════════════════════════
// Target Configuration
// ═══════════════════════════════════════════════════════════════════════════

bool nova_llvm_configure_target(NovaLLVMContext *ctx, NovaTargetArch arch) {
  if (!ctx)
    return false;

  const char *triple = NULL;
  switch (arch) {
  case NOVA_ARCH_NATIVE:  triple = ctx->target_triple;               break;
  case NOVA_ARCH_X86_64:  triple = "x86_64-unknown-linux-gnu";       break;
  case NOVA_ARCH_ARM64:   triple = "aarch64-unknown-linux-gnu";      break;
  case NOVA_ARCH_WASM:    triple = "wasm32-unknown-unknown";         break;
  case NOVA_ARCH_RISCV:   triple = "riscv64-unknown-linux-gnu";      break;
  default:
    return false;
  }

  LLVMSetTarget(ctx->module, triple);
  ctx->target_arch = arch;
  return true;
}

/*
 * FIX #8: Optimization pass manager ported to the LLVM 17+ PassBuilder API.
 * The old per-pass functions were deprecated; use LLVMRunPasses() instead.
 */
void nova_llvm_enable_optimizations(NovaLLVMContext *ctx, NovaOptLevel level) {
  if (!ctx)
    return;

  ctx->opt_level = level;

#if LLVM_VERSION_MAJOR >= 17
  /* New Pass Manager (LLVM 17+): build a pipeline string then run it. */
  const char *pipeline = NULL;
  switch (level) {
  case NOVA_OPT_NONE: return;          /* nothing to do */
  case NOVA_OPT_O1:   pipeline = "default<O1>"; break;
  case NOVA_OPT_O2:   pipeline = "default<O2>"; break;
  case NOVA_OPT_O3:   pipeline = "default<O3>"; break;
  default:            pipeline = "default<O2>"; break;
  }

  LLVMPassBuilderOptionsRef opts = LLVMCreatePassBuilderOptions();
  LLVMPassBuilderOptionsSetLoopUnrolling(opts, level >= NOVA_OPT_O2);
  LLVMPassBuilderOptionsSetMergeFunctions(opts, level >= NOVA_OPT_O3);

  char *error = NULL;
  LLVMTargetRef target_ref = NULL;
  if (!LLVMGetTargetFromTriple(ctx->target_triple, &target_ref, &error)) {
    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
        target_ref, ctx->target_triple, ctx->cpu_name, ctx->features,
        LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);

    LLVMErrorRef err = LLVMRunPasses(ctx->module, pipeline, machine, opts);
    if (err) {
      char *msg = LLVMGetErrorMessage(err);
      fprintf(stderr, "Optimization error: %s\n", msg);
      LLVMDisposeErrorMessage(msg);
      LLVMConsumeError(err);
    }
    LLVMDisposeTargetMachine(machine);
  } else {
    fprintf(stderr, "enable_optimizations: bad triple: %s\n", error);
    LLVMDisposeMessage(error);
  }
  LLVMDisposePassBuilderOptions(opts);

#else
  /* Legacy Pass Manager (LLVM < 17) */
  if (!ctx->mpm)
    ctx->mpm = LLVMCreatePassManager();

  if (level >= NOVA_OPT_O1) {
    LLVMAddPromoteMemoryToRegisterPass(ctx->mpm);
    LLVMAddInstructionCombiningPass(ctx->mpm);
    LLVMAddReassociatePass(ctx->mpm);
    LLVMAddGVNPass(ctx->mpm);
    LLVMAddCFGSimplificationPass(ctx->mpm);
  }
  if (level >= NOVA_OPT_O2) {
    LLVMAddConstantPropagationPass(ctx->mpm);
    LLVMAddDeadStoreEliminationPass(ctx->mpm);
    LLVMAddAggressiveDCEPass(ctx->mpm);
  }
  if (level >= NOVA_OPT_O3) {
    LLVMAddFunctionInliningPass(ctx->mpm);
    LLVMAddTailCallEliminationPass(ctx->mpm);
  }
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// Code Generation & Verification
// ═══════════════════════════════════════════════════════════════════════════

bool nova_llvm_verify_module(NovaLLVMContext *ctx) {
  if (!ctx || !ctx->module)
    return false;

  char *error = NULL;
  if (LLVMVerifyModule(ctx->module, LLVMReturnStatusAction, &error)) {
    fprintf(stderr, "LLVM Module verification failed:\n%s\n", error);
    LLVMDisposeMessage(error);
    return false;
  }
  return true;
}

void nova_llvm_dump_ir(NovaLLVMContext *ctx) {
  if (!ctx || !ctx->module)
    return;
  LLVMDumpModule(ctx->module);
}

// ═══════════════════════════════════════════════════════════════════════════
// JIT Compilation & Execution
// ═══════════════════════════════════════════════════════════════════════════

bool nova_llvm_compile_jit(NovaLLVMContext *ctx) {
  if (!ctx || !ctx->module)
    return false;

  if (!nova_llvm_verify_module(ctx))
    return false;

  /* Legacy pass manager opt (no-op on LLVM 17+ path — already applied) */
  if (ctx->mpm && ctx->opt_level > NOVA_OPT_NONE)
    LLVMRunPassManager(ctx->mpm, ctx->module);

  char *error = NULL;
  /*
   * FIX #1: Mark engine_owns_module BEFORE the call so that if the call
   * succeeds the destroy path is correct, and if it fails ctx->engine
   * stays NULL so the module is still freed separately.
   */
  if (LLVMCreateMCJITCompilerForModule(&ctx->engine, ctx->module, NULL, 0,
                                       &error)) {
    fprintf(stderr, "Failed to create JIT: %s\n", error);
    LLVMDisposeMessage(error);
    ctx->engine = NULL;
    return false;
  }

  ctx->engine_owns_module = true;
  return true;
}

/*
 * FIX #10: Use LLVMGetFunctionAddress (modern, avoids deprecated
 * LLVMGenericValueRef API which has broken float/struct support).
 */
int64_t nova_llvm_run_function(NovaLLVMContext *ctx, const char *func_name,
                               int64_t *args, int arg_count) {
  if (!ctx || !ctx->engine)
    return -1;

  uint64_t addr = LLVMGetFunctionAddress(ctx->engine, func_name);
  if (!addr) {
    fprintf(stderr, "Function '%s' not found\n", func_name);
    return -1;
  }

  /*
   * Cast to a generic int64_t(*)(int64_t, ...) and call directly.
   * For functions with different signatures callers should get the address
   * and cast themselves — this helper covers the common i64→i64 case.
   */
  typedef int64_t (*fn0_t)(void);
  typedef int64_t (*fn1_t)(int64_t);
  typedef int64_t (*fn2_t)(int64_t, int64_t);
  typedef int64_t (*fn3_t)(int64_t, int64_t, int64_t);

  switch (arg_count) {
  case 0: return ((fn0_t)(uintptr_t)addr)();
  case 1: return ((fn1_t)(uintptr_t)addr)(args[0]);
  case 2: return ((fn2_t)(uintptr_t)addr)(args[0], args[1]);
  case 3: return ((fn3_t)(uintptr_t)addr)(args[0], args[1], args[2]);
  default:
    fprintf(stderr, "nova_llvm_run_function: >3 args unsupported\n");
    return -1;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// IR Generation Helpers
// ═══════════════════════════════════════════════════════════════════════════

LLVMValueRef nova_llvm_build_function(NovaLLVMContext *ctx, const char *name,
                                      LLVMTypeRef ret_type,
                                      LLVMTypeRef *param_types,
                                      int param_count) {
  if (!ctx)
    return NULL;

  LLVMTypeRef func_type =
      LLVMFunctionType(ret_type, param_types, (unsigned)param_count, 0);
  LLVMValueRef func = LLVMAddFunction(ctx->module, name, func_type);

  /* FIX #4: Use context-aware variant to avoid global-context pollution */
  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(ctx->context, func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, entry);

  return func;
}

LLVMValueRef nova_llvm_const_int(NovaLLVMContext *ctx, int64_t val) {
  if (!ctx)
    return NULL;
  return LLVMConstInt(LLVMInt64TypeInContext(ctx->context),
                      (unsigned long long)val, 1);
}

#else // !HAVE_LLVM

NovaLLVMContext *nova_llvm_create_context(const char *module_name) {
  (void)module_name;
  fprintf(stderr, "LLVM support not compiled in\n");
  return NULL;
}
void nova_llvm_destroy_context(NovaLLVMContext *ctx)                      { (void)ctx; }
bool nova_llvm_configure_target(NovaLLVMContext *ctx, NovaTargetArch arch){ (void)ctx; (void)arch; return false; }
void nova_llvm_enable_optimizations(NovaLLVMContext *ctx, NovaOptLevel l) { (void)ctx; (void)l; }
bool nova_llvm_verify_module(NovaLLVMContext *ctx)                        { (void)ctx; return false; }
void nova_llvm_dump_ir(NovaLLVMContext *ctx)                              { (void)ctx; }
bool nova_llvm_compile_jit(NovaLLVMContext *ctx)                          { (void)ctx; return false; }
int64_t nova_llvm_run_function(NovaLLVMContext *ctx, const char *fn,
                               int64_t *a, int c)
{ (void)ctx; (void)fn; (void)a; (void)c; return -1; }

#endif // HAVE_LLVM
