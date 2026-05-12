/**
 * Nova LLVM Emit Functions
 * Object file and assembly generation
 */

#include "nova_llvm_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LLVM
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

bool nova_llvm_emit_object(NovaLLVMContext *ctx, const char *filename) {
  if (!ctx || !filename)
    return false;

  // Get target from triple
  char *error = NULL;
  LLVMTargetRef target;

  if (LLVMGetTargetFromTriple(ctx->target_triple, &target, &error)) {
    fprintf(stderr, "Failed to get target: %s\n", error);
    LLVMDisposeMessage(error);
    return false;
  }
  // Create target machine
  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, ctx->target_triple, ctx->cpu_name, ctx->features,
      LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);

  if (!machine) {
    fprintf(stderr, "Failed to create target machine\n");
    return false;
  }

  // Important: Set data layout for the module
  LLVMTargetDataRef data_layout = LLVMCreateTargetDataLayout(machine);
  char *data_layout_str = LLVMCopyStringRepOfTargetData(data_layout);
  LLVMSetDataLayout(ctx->module, data_layout_str);
  LLVMDisposeMessage(data_layout_str);
  LLVMDisposeTargetData(data_layout);

  // Verify module before emitting
  char *verify_error = NULL;
  if (LLVMVerifyModule(ctx->module, LLVMReturnStatusAction, &verify_error)) {
    fprintf(stderr, "Module verification failed: %s\n", verify_error);
    if (verify_error)
      LLVMDisposeMessage(verify_error);
    LLVMDisposeTargetMachine(machine);
    return false;
  }
  if (LLVMTargetMachineEmitToFile(machine, ctx->module, (char *)filename,
                                  LLVMObjectFile, &error)) {
    fprintf(stderr, "Failed to emit object file: %s\n", error);
    LLVMDisposeMessage(error);
    LLVMDisposeTargetMachine(machine);
    return false;
  }

  LLVMDisposeTargetMachine(machine);
  return true;
}

bool nova_llvm_emit_assembly(NovaLLVMContext *ctx, const char *filename) {
  if (!ctx || !filename)
    return false;

  char *error = NULL;
  LLVMTargetRef target;

  if (LLVMGetTargetFromTriple(ctx->target_triple, &target, &error)) {
    fprintf(stderr, "Failed to get target: %s\n", error);
    LLVMDisposeMessage(error);
    return false;
  }

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, ctx->target_triple, ctx->cpu_name, ctx->features,
      LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);

  if (!machine) {
    fprintf(stderr, "Failed to create target machine\n");
    return false;
  }

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
  if (!ctx)
    return;

  switch (level) {
  case 0:
    ctx->opt_level = NOVA_OPT_NONE;
    break;
  case 1:
    ctx->opt_level = NOVA_OPT_O1;
    break;
  case 2:
    ctx->opt_level = NOVA_OPT_O2;
    break;
  case 3:
    ctx->opt_level = NOVA_OPT_O3;
    break;
  default:
    ctx->opt_level = NOVA_OPT_O2;
    break;
  }
}

#else // !HAVE_LLVM

bool nova_llvm_emit_object(NovaLLVMContext *ctx, const char *filename) {
  (void)ctx;
  (void)filename;
  fprintf(stderr, "LLVM support not compiled in\n");
  return false;
}

bool nova_llvm_emit_assembly(NovaLLVMContext *ctx, const char *filename) {
  (void)ctx;
  (void)filename;
  fprintf(stderr, "LLVM support not compiled in\n");
  return false;
}

void nova_llvm_set_opt_level(NovaLLVMContext *ctx, int level) {
  (void)ctx;
  (void)level;
}

#endif // HAVE_LLVM
