/**
 * @file bench_compiler.c
 * @brief Benchmark for compiler
 */

#include <stdio.h>
#include <time.h>

int main(void)
{
    printf("Running bench_compiler...\n");
    clock_t start = clock();
    
    /* TODO: Implement benchmark */
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time: %.3f seconds\n", time_spent);
    
    return 0;
}
