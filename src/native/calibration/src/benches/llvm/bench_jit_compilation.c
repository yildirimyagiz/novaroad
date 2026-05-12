/**
 * LLVM JIT Compilation Benchmark
 * Measures compilation and optimization times
 */

#include "nova_autocal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void *llvm_jit_compile(const char *ir_code, int opt_level);
extern void llvm_jit_destroy(void *jit_module);
extern void *llvm_jit_get_function(void *jit_module, const char *name);

static const char *test_ir_simple = 
    "define i32 @add(i32 %a, i32 %b) {\n"
    "  %result = add i32 %a, %b\n"
    "  ret i32 %result\n"
    "}\n";

static const char *test_ir_complex = 
    "define double @mandelbrot(double %x, double %y) {\n"
    "entry:\n"
    "  br label %loop\n"
    "loop:\n"
    "  %zr = phi double [0.0, %entry], [%zr_new, %loop]\n"
    "  %zi = phi double [0.0, %entry], [%zi_new, %loop]\n"
    "  %iter = phi i32 [0, %entry], [%iter_new, %loop]\n"
    "  %zr2 = fmul double %zr, %zr\n"
    "  %zi2 = fmul double %zi, %zi\n"
    "  %sum = fadd double %zr2, %zi2\n"
    "  %cmp = fcmp olt double %sum, 4.0\n"
    "  %cmp2 = icmp slt i32 %iter, 100\n"
    "  %continue = and i1 %cmp, %cmp2\n"
    "  br i1 %continue, label %loop_body, label %exit\n"
    "loop_body:\n"
    "  %zr_new = fadd double %zr2, %x\n"
    "  %zr_new2 = fsub double %zr_new, %zi2\n"
    "  %zi_tmp = fmul double %zr, %zi\n"
    "  %zi_new = fadd double %zi_tmp, %zi_tmp\n"
    "  %zi_new2 = fadd double %zi_new, %y\n"
    "  %iter_new = add i32 %iter, 1\n"
    "  br label %loop\n"
    "exit:\n"
    "  %result = sitofp i32 %iter to double\n"
    "  ret double %result\n"
    "}\n";

typedef struct {
    const char *ir_code;
    int opt_level;
} JITContext;

static void run_jit_compile(void *ctx) {
    JITContext *jc = (JITContext *)ctx;
    void *module = llvm_jit_compile(jc->ir_code, jc->opt_level);
    llvm_jit_destroy(module);
}

int main(void) {
    printf("=== LLVM JIT Compilation Benchmark ===\n");
    
    AutocalContext *ctx = autocal_create();
    
    const char *test_cases[] = {"Simple", "Complex"};
    const char *ir_codes[] = {test_ir_simple, test_ir_complex};
    
    for (int t = 0; t < 2; t++) {
        printf("\n%s IR:\n", test_cases[t]);
        
        for (int opt_level = 0; opt_level <= 3; opt_level++) {
            JITContext jc = {ir_codes[t], opt_level};
            
            double time_ms = autocal_measure_time(
                (AutocalWorkload){
                    .name = "jit_compile",
                    .execute = run_jit_compile,
                    .context = &jc,
                    .iterations = 50
                }
            );
            
            printf("  -O%d: %.3f ms\n", opt_level, time_ms);
        }
    }
    
    autocal_destroy(ctx);
    return 0;
}
