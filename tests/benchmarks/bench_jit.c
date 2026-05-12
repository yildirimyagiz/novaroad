/**
 * @file bench_jit.c
 * @brief Benchmark for jit
 */

#include <stdio.h>
#include <time.h>

int main(void)
{
    printf("Running bench_jit...\n");
    clock_t start = clock();
    
    /* TODO: Implement benchmark */
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time: %.3f seconds\n", time_spent);
    
    return 0;
}
