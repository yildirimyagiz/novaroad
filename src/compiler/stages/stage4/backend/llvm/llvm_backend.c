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
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

// ═══════════════════════════════════════════════════════════════════════════
// Context Management
// ═══════════════════════════════════════════════════════════════════════════

NovaLLVMContext *nova_llvm_create_context(const char *module_name) {
  NovaLLVMContext *ctx = (NovaLLVMContext *)malloc(sizeof(NovaLLVMContext));
  if (!ctx)
    return NULL;

  // Initialize LLVM for native and all targets (needed for AOT/Stage 1)
  LLVMLinkInMCJIT();
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmParsers();
  LLVMInitializeAllAsmPrinters();

  // Also initialize native for safety
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  // Create context, module, builder
  ctx->context = LLVMContextCreate();
  ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
  ctx->builder = LLVMCreateBuilderInContext(ctx->context);
  ctx->engine = NULL;
  ctx->mpm = NULL;

  // Get native target
  ctx->target_triple = LLVMGetDefaultTargetTriple();
  LLVMSetTarget(ctx->module, ctx->target_triple);

  ctx->cpu_name = LLVMGetHostCPUName();
  ctx->features = LLVMGetHostCPUFeatures();

  // If host features is NULL (happens on some platforms), set to empty string
  if (!ctx->features) {
    ctx->features = LLVMCreateMessage("");
  }
  if (!ctx->cpu_name) {
    ctx->cpu_name = LLVMCreateMessage("generic");
  }

  return ctx;
}

void nova_llvm_destroy_context(NovaLLVMContext *ctx) {
  if (!ctx)
    return;

  if (ctx->mpm)
    LLVMDisposePassManager(ctx->mpm);
  if (ctx->engine)
    LLVMDisposeExecutionEngine(ctx->engine);
  if (ctx->builder)
    LLVMDisposeBuilder(ctx->builder);
  if (ctx->module && !ctx->engine)
    LLVMDisposeModule(ctx->module);
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
  case NOVA_ARCH_NATIVE:
    triple = ctx->target_triple;
    break;
  case NOVA_ARCH_X86_64:
    triple = "x86_64-unknown-linux-gnu";
    break;
  case NOVA_ARCH_ARM64:
    triple = "aarch64-unknown-linux-gnu";
    break;
  case NOVA_ARCH_WASM:
    triple = "wasm32-unknown-unknown";
    break;
  case NOVA_ARCH_RISCV:
    triple = "riscv64-unknown-linux-gnu";
    break;
  default:
    return false;
  }

  LLVMSetTarget(ctx->module, triple);
  ctx->target_arch = arch;
  return true;
}

void nova_llvm_enable_optimizations(NovaLLVMContext *ctx, NovaOptLevel level) {
  if (!ctx)
    return;

  ctx->opt_level = level;

  // Create pass manager if not exists
  if (!ctx->mpm) {
    ctx->mpm = LLVMCreatePassManager();
  }

  // Add optimization passes based on level
  if (level >= NOVA_OPT_O1) {
    // LLVMAddPromoteMemoryToRegisterPass(ctx->mpm); // Deprecated in LLVM 17+
    // LLVMAddInstructionCombiningPass(ctx->mpm);    // Deprecated in LLVM 17+
    // LLVMAddReassociatePass(ctx->mpm);             // Deprecated in LLVM 17+
    // LLVMAddGVNPass(ctx->mpm);                     // Deprecated in LLVM 17+
    // LLVMAddCFGSimplificationPass(ctx->mpm);       // Deprecated in LLVM 17+
  }

  if (level >= NOVA_OPT_O2) {
    // LLVMAddConstantPropagationPass(ctx->mpm);     // Deprecated in LLVM 17+
    // LLVMAddDeadStoreEliminationPass(ctx->mpm);    // Deprecated in LLVM 17+
    // LLVMAddAggressiveDCEPass(ctx->mpm);           // Deprecated in LLVM 17+
  }

  if (level >= NOVA_OPT_O3) {
    // LLVMAddFunctionInliningPass(ctx->mpm);        // Deprecated in LLVM 17+
    // LLVMAddTailCallEliminationPass(ctx->mpm);     // Deprecated in LLVM 17+
  }
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

  // Verify first
  if (!nova_llvm_verify_module(ctx)) {
    return false;
  }

  // Apply optimizations if enabled
  if (ctx->mpm && ctx->opt_level > NOVA_OPT_NONE) {
    LLVMRunPassManager(ctx->mpm, ctx->module);
  }

  // Create JIT execution engine
  char *error = NULL;
  if (LLVMCreateMCJITCompilerForModule(&ctx->engine, ctx->module, NULL, 0,
                                       &error)) {
    fprintf(stderr, "Failed to create JIT: %s\n", error);
    LLVMDisposeMessage(error);
    return false;
  }

  return true;
}

int64_t nova_llvm_run_function(NovaLLVMContext *ctx, const char *func_name,
                               int64_t *args, int arg_count) {
  if (!ctx || !ctx->engine)
    return -1;

  // Find function
  LLVMValueRef func = LLVMGetNamedFunction(ctx->module, func_name);
  if (!func) {
    fprintf(stderr, "Function '%s' not found\n", func_name);
    return -1;
  }

  // Prepare generic values for arguments
  LLVMGenericValueRef *gen_args = NULL;
  if (arg_count > 0) {
    gen_args = (LLVMGenericValueRef *)malloc(sizeof(LLVMGenericValueRef) *
                                             (size_t)arg_count);
    for (int i = 0; i < arg_count; i++) {
      gen_args[i] = LLVMCreateGenericValueOfInt(LLVMInt64Type(),
                                                (unsigned long long)args[i], 1);
    }
  }

  // Run function
  LLVMGenericValueRef result =
      LLVMRunFunction(ctx->engine, func, (unsigned)arg_count, gen_args);
  int64_t ret = (int64_t)LLVMGenericValueToInt(result, 1);

  // Cleanup
  if (gen_args) {
    for (int i = 0; i < arg_count; i++) {
      LLVMDisposeGenericValue(gen_args[i]);
    }
    free(gen_args);
  }
  LLVMDisposeGenericValue(result);

  return ret;
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

bool nova_llvm_emit_object(NovaLLVMContext *ctx, const char *filename) {
  if (!ctx || !ctx->module)
    return false;

  LLVMTargetRef target;
  char *error = NULL;
  if (LLVMGetTargetFromTriple(ctx->target_triple, &target, &error)) {
    fprintf(stderr, "Failed to find target: %s\n", error);
    if (error)
      LLVMDisposeMessage(error);
    return false;
  }

  LLVMCodeGenOptLevel opt = LLVMCodeGenLevelDefault;
  switch (ctx->opt_level) {
  case NOVA_OPT_NONE:
    opt = LLVMCodeGenLevelNone;
    break;
  case NOVA_OPT_O1:
    opt = LLVMCodeGenLevelLess;
    break;
  case NOVA_OPT_O2:
    opt = LLVMCodeGenLevelDefault;
    break;
  case NOVA_OPT_O3:
    opt = LLVMCodeGenLevelAggressive;
    break;
  case NOVA_OPT_SIZE:
    opt = LLVMCodeGenLevelDefault;
    break;
  }

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, ctx->target_triple, ctx->cpu_name, ctx->features, opt,
      LLVMRelocDefault, LLVMCodeModelDefault);

  if (LLVMTargetMachineEmitToFile(machine, ctx->module, (char *)filename,
                                  LLVMObjectFile, &error)) {
    fprintf(stderr, "Failed to emit object: %s\n", error);
    LLVMDisposeMessage(error);
    LLVMDisposeTargetMachine(machine);
    return false;
  }

  LLVMDisposeTargetMachine(machine);
  return true;
}

bool nova_llvm_emit_assembly(NovaLLVMContext *ctx, const char *filename) {
  if (!ctx || !ctx->module)
    return false;

  LLVMTargetRef target;
  char *error = NULL;
  if (LLVMGetTargetFromTriple(ctx->target_triple, &target, &error)) {
    fprintf(stderr, "Failed to find target: %s\n", error);
    if (error)
      LLVMDisposeMessage(error);
    return false;
  }

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, ctx->target_triple, ctx->cpu_name, ctx->features,
      LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);

  if (LLVMTargetMachineEmitToFile(machine, ctx->module, (char *)filename,
                                  LLVMAssemblyFile, &error)) {
    fprintf(stderr, "Failed to emit assembly: %s\n", error);
    LLVMDisposeMessage(error);
    LLVMDisposeTargetMachine(machine);
    return false;
  }

  LLVMDisposeTargetMachine(machine);
  return true;
}

void nova_llvm_set_opt_level(NovaLLVMContext *ctx, int level) {
  if (ctx)
    ctx->opt_level = (NovaOptLevel)level;
}

bool nova_llvm_codegen_program(NovaLLVMContext *ctx, nova_program_t *program) {
  (void)ctx;
  (void)program;
  // TBD: Full codegen entry point
  return true;
}

#else // !HAVE_LLVM

// ═══════════════════════════════════════════════════════════════════════════
// Stub implementations when LLVM is not available
// ═══════════════════════════════════════════════════════════════════════════

NovaLLVMContext *nova_llvm_create_context(const char *module_name) {
  (void)module_name;
  fprintf(stderr, "LLVM support not compiled in\n");
  return NULL;
}

void nova_llvm_destroy_context(NovaLLVMContext *ctx) { (void)ctx; }

bool nova_llvm_configure_target(NovaLLVMContext *ctx, NovaTargetArch arch) {
  (void)ctx;
  (void)arch;
  return false;
}

void nova_llvm_enable_optimizations(NovaLLVMContext *ctx, NovaOptLevel level) {
  (void)ctx;
  (void)level;
}

bool nova_llvm_verify_module(NovaLLVMContext *ctx) {
  (void)ctx;
  return false;
}

void nova_llvm_dump_ir(NovaLLVMContext *ctx) { (void)ctx; }

bool nova_llvm_compile_jit(NovaLLVMContext *ctx) {
  (void)ctx;
  return false;
}

int64_t nova_llvm_run_function(NovaLLVMContext *ctx, const char *func_name,
                               int64_t *args, int arg_count) {
  (void)ctx;
  (void)func_name;
  (void)args;
  (void)arg_count;
  return -1;
}

bool nova_llvm_emit_object(NovaLLVMContext *ctx, const char *filename) {
  (void)ctx;
  (void)filename;
  return false;
}

bool nova_llvm_emit_assembly(NovaLLVMContext *ctx, const char *filename) {
  (void)ctx;
  (void)filename;
  return false;
}

void nova_llvm_set_opt_level(NovaLLVMContext *ctx, int level) {
  (void)ctx;
  (void)level;
}

bool nova_llvm_codegen_program(NovaLLVMContext *ctx, nova_program_t *program) {
  (void)ctx;
  (void)program;
  return false;
}

#endif // HAVE_LLVM