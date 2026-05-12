// B.1 IR Translation - Nova AST to LLVM IR
// LLVM backend implementation for Nova compiler

#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations from Nova AST
typedef struct nova_type nova_type_t;
typedef struct nova_expr nova_expr_t;
typedef struct nova_stmt nova_stmt_t;

// LLVM IR Generator context
typedef struct nova_llvm_context {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMTargetMachineRef target_machine;

    // Type cache
    LLVMTypeRef int32_type;
    LLVMTypeRef void_type;

    // Value cache for variables
    // TODO: Add symbol table integration
} nova_llvm_context_t;

// Create LLVM context
nova_llvm_context_t* nova_llvm_create_context(const char* module_name) {
    nova_llvm_context_t* ctx = calloc(1, sizeof(nova_llvm_context_t));

    // Initialize LLVM
    LLVMInitializeCore(LLVMGetGlobalPassRegistry());
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    ctx->context = LLVMContextCreate();
    ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
    ctx->builder = LLVMCreateBuilderInContext(ctx->context);

    // Setup target machine
    LLVMTargetRef target = LLVMGetFirstTarget();
    const char* triple = LLVMGetDefaultTargetTriple();
    LLVMSetTarget(ctx->module, triple);

    char* error = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Failed to get target: %s\n", error);
        LLVMDisposeMessage(error);
        return NULL;
    }

    ctx->target_machine = LLVMCreateTargetMachine(
        target, triple, "", "",
        LLVMCodeGenLevelDefault,
        LLVMRelocDefault,
        LLVMCodeModelDefault
    );

    // Cache common types
    ctx->int32_type = LLVMInt32TypeInContext(ctx->context);
    ctx->void_type = LLVMVoidTypeInContext(ctx->context);

    printf("🔧 LLVM Context created for module: %s\n", module_name);
    return ctx;
}

// Destroy LLVM context
void nova_llvm_destroy_context(nova_llvm_context_t* ctx) {
    if (!ctx) return;

    if (ctx->target_machine) LLVMDisposeTargetMachine(ctx->target_machine);
    if (ctx->builder) LLVMDisposeBuilder(ctx->builder);
    if (ctx->module) LLVMDisposeModule(ctx->module);
    if (ctx->context) LLVMContextDispose(ctx->context);

    free(ctx);
}

// Type translation: Nova type -> LLVM type
LLVMTypeRef nova_llvm_translate_type(nova_llvm_context_t* ctx, nova_type_t* nova_type) {
    // Simplified: only handle basic types for now
    if (!nova_type) return ctx->void_type;

    // TODO: Add proper type translation based on nova_type->kind
    // For now, assume everything is int32
    return ctx->int32_type;
}

// Expression translation: Nova expr -> LLVM value
LLVMValueRef nova_llvm_translate_expr(nova_llvm_context_t* ctx, nova_expr_t* expr) {
    if (!expr) return NULL;

    switch (expr->kind) {
        case 0: // Assume EXPR_LIT_INT
            // TODO: Get actual literal value from AST
            return LLVMConstInt(ctx->int32_type, 42, 0); // Dummy value

        case 1: // Assume EXPR_IDENT
            // TODO: Look up variable in symbol table
            printf("⚠️  Variable lookup not implemented yet\n");
            return LLVMConstInt(ctx->int32_type, 0, 0); // Dummy

        default:
            printf("⚠️  Expression type %d not implemented\n", expr->kind);
            return LLVMConstInt(ctx->int32_type, 0, 0);
    }
}

// Statement translation: Nova stmt -> LLVM instructions
void nova_llvm_translate_stmt(nova_llvm_context_t* ctx, nova_stmt_t* stmt, LLVMValueRef function) {
    if (!stmt) return;

    switch (stmt->kind) {
        case 0: // Assume STMT_EXPR
            if (stmt->as.expr_stmt) {
                LLVMValueRef value = nova_llvm_translate_expr(ctx, stmt->as.expr_stmt);
                // Expression statements don't produce values in LLVM
                (void)value; // Suppress unused variable warning
            }
            break;

        case 1: // Assume STMT_VAR
            // TODO: Allocate variable and store initial value
            printf("⚠️  Variable declaration not implemented yet\n");
            break;

        default:
            printf("⚠️  Statement type %d not implemented\n", stmt->kind);
            break;
    }
}

// Function translation: Create LLVM function from Nova function
LLVMValueRef nova_llvm_translate_function(nova_llvm_context_t* ctx, const char* func_name,
                                        nova_type_t* return_type, nova_stmt_t* body) {
    // Create function type: () -> return_type
    LLVMTypeRef ret_type = nova_llvm_translate_type(ctx, return_type);
    LLVMTypeRef func_type = LLVMFunctionType(ret_type, NULL, 0, 0);

    // Create function
    LLVMValueRef function = LLVMAddFunction(ctx->module, func_name, func_type);

    // Create entry block
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx->context, function, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);

    // Translate function body
    nova_llvm_translate_stmt(ctx, body, function);

    // Add implicit return (void functions get implicit return)
    if (LLVMGetReturnType(ret_type) == ctx->void_type) {
        LLVMBuildRetVoid(ctx->builder);
    } else {
        // For non-void functions, add dummy return
        LLVMBuildRet(ctx->builder, LLVMConstInt(ret_type, 0, 0));
    }

    return function;
}

// Module translation: Create LLVM module from Nova program
int nova_llvm_translate_module(nova_llvm_context_t* ctx) {
    printf("🏗️  Translating Nova module to LLVM IR...\n");

    // Create main function as example
    // TODO: Parse actual Nova functions from AST
    LLVMValueRef main_func = nova_llvm_translate_function(ctx, "main", NULL, NULL);

    // Verify module
    char* error = NULL;
    if (LLVMVerifyModule(ctx->module, LLVMPrintMessageAction, &error) != 0) {
        fprintf(stderr, "Module verification failed: %s\n", error);
        LLVMDisposeMessage(error);
        return 1;
    }

    printf("✅ LLVM IR translation completed\n");
    return 0;
}

// Generate object file
int nova_llvm_emit_object_file(nova_llvm_context_t* ctx, const char* output_path) {
    printf("📦 Emitting object file: %s\n", output_path);

    char* error = NULL;
    if (LLVMTargetMachineEmitToFile(ctx->target_machine, ctx->module,
                                   output_path, LLVMObjectFile, &error) != 0) {
        fprintf(stderr, "Failed to emit object file: %s\n", error);
        LLVMDisposeMessage(error);
        return 1;
    }

    printf("✅ Object file generated successfully\n");
    return 0;
}

// Print LLVM IR (for debugging)
void nova_llvm_dump_ir(nova_llvm_context_t* ctx) {
    printf("📄 LLVM IR:\n");
    printf("---\n");
    LLVMDumpModule(ctx->module);
    printf("---\n");
}

// Test B.1 IR Translation
int test_b1_ir_translation() {
    printf("=== B.1 IR Translation Test ===\n\n");

    // Create LLVM context
    nova_llvm_context_t* ctx = nova_llvm_create_context("nova_test");
    if (!ctx) {
        printf("❌ Failed to create LLVM context\n");
        return 1;
    }

    // Translate module (creates dummy main function)
    if (nova_llvm_translate_module(ctx) != 0) {
        printf("❌ IR translation failed\n");
        nova_llvm_destroy_context(ctx);
        return 1;
    }

    // Dump IR for inspection
    nova_llvm_dump_ir(ctx);

    // Try to emit object file
    if (nova_llvm_emit_object_file(ctx, "nova_test.o") != 0) {
        printf("❌ Object file emission failed\n");
        nova_llvm_destroy_context(ctx);
        return 1;
    }

    // Clean up
    nova_llvm_destroy_context(ctx);

    printf("\n✅ B.1 IR translation test completed successfully\n");
    printf("   Generated: nova_test.o\n");

    return 0;
}

int main() {
    return test_b1_ir_translation();
}
