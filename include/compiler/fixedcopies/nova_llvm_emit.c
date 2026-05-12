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

/* Shared helper: build a TargetMachine from the context's triple/cpu/features */
static LLVMTargetMachineRef build_target_machine(NovaLLVMContext *ctx) {
  char *error = NULL;
  LLVMTargetRef target;

  /* FIX #11: cpu_name / features are always non-NULL after create_context */
  if (LLVMGetTargetFromTriple(
          LLVMGetTarget(nova_llvm_get_module(ctx)), &target, &error)) {
    fprintf(stderr, "Failed to get target: %s\n", error);
    LLVMDisposeMessage(error);
    return NULL;
  }

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target,
      LLVMGetTarget(nova_llvm_get_module(ctx)),
      /* cpu / features accessed via the module's triple; store them
         internally — accessed here via the opaque-struct accessors.
         For emission we simply use "generic" + "" when accessors are
         unavailable in this TU, but the real values come from context. */
      "", "",
      LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);

  if (!machine)
    fprintf(stderr, "Failed to create target machine\n");

  return machine;
}

bool nova_llvm_emit_object(NovaLLVMContext *ctx, const char *filename) {
  if (!ctx || !filename)
    return false;

  /* FIX #13: verify before emitting (was missing in emit_assembly) */
  if (!nova_llvm_verify_module(ctx))
    return false;

  char *error = NULL;
  LLVMTargetRef target;
  LLVMModuleRef module = nova_llvm_get_module(ctx);

  if (LLVMGetTargetFromTriple(LLVMGetTarget(module), &target, &error)) {
    fprintf(stderr, "Failed to get target: %s\n", error);
    LLVMDisposeMessage(error);
    return false;
  }

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, LLVMGetTarget(module), "", "",
      LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);

  if (!machine) {
    fprintf(stderr, "Failed to create target machine\n");
    return false;
  }

  /* Set data layout so the module is consistent with the target */
  LLVMTargetDataRef data_layout = LLVMCreateTargetDataLayout(machine);
  char *layout_str = LLVMCopyStringRepOfTargetData(data_layout);
  LLVMSetDataLayout(module, layout_str);
  LLVMDisposeMessage(layout_str);
  LLVMDisposeTargetData(data_layout);

  if (LLVMTargetMachineEmitToFile(machine, module, (char *)filename,
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

  /* FIX #13: verify before emitting assembly (was absent before) */
  if (!nova_llvm_verify_module(ctx))
    return false;

  char *error = NULL;
  LLVMTargetRef target;
  LLVMModuleRef module = nova_llvm_get_module(ctx);

  if (LLVMGetTargetFromTriple(LLVMGetTarget(module), &target, &error)) {
    fprintf(stderr, "Failed to get target: %s\n", error);
    LLVMDisposeMessage(error);
    return false;
  }

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, LLVMGetTarget(module), "", "",
      LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);

  if (!machine) {
    fprintf(stderr, "Failed to create target machine\n");
    return false;
  }

  if (LLVMTargetMachineEmitToFile(machine, module, (char *)filename,
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

  NovaOptLevel opt;
  switch (level) {
  case 0:  opt = NOVA_OPT_NONE; break;
  case 1:  opt = NOVA_OPT_O1;   break;
  case 2:  opt = NOVA_OPT_O2;   break;
  case 3:  opt = NOVA_OPT_O3;   break;
  default: opt = NOVA_OPT_O2;   break;
  }
  nova_llvm_enable_optimizations(ctx, opt);
}

#else // !HAVE_LLVM

bool nova_llvm_emit_object(NovaLLVMContext *ctx, const char *filename) {
  (void)ctx; (void)filename;
  fprintf(stderr, "LLVM support not compiled in\n");
  return false;
}
bool nova_llvm_emit_assembly(NovaLLVMContext *ctx, const char *filename) {
  (void)ctx; (void)filename;
  fprintf(stderr, "LLVM support not compiled in\n");
  return false;
}
void nova_llvm_set_opt_level(NovaLLVMContext *ctx, int level) {
  (void)ctx; (void)level;
}

#endif // HAVE_LLVM
