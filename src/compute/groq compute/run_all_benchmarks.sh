#!/bin/bash

executables=(
    "adaptive_matmul"
    "advanced_matmul"
    "benchmark"
    "comprehensive_real_test"
    "extreme_matmul"
    "final_matmul"
    "groq_compute"
    "optimized_matmul"
    "real_benchmark"
    "real_valid_benchmark"
    "ultra_advanced_matmul"
    "ultra_advanced_matmul_transpose"
    "ultra_matmul_transpose"
    "ultra_tuned_matmul"
    "adaptive_profile_matmul"
)

echo "Running benchmarks for all matmul implementations..."
echo "==============================================="

for exe in "${executables[@]}"; do
    if [ -x "$exe" ]; then
        echo "Running $exe..."
        output=$(./"$exe")
        echo "$output"
        echo "-----------------------------------------------"
    else
        echo "$exe is not executable or not found."
    fi
done

echo "All benchmarks completed."
