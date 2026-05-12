# Nova Scientific Performance & Efficiency Validation Suite

## Overview

This is a **non-simulated, hardware-realistic performance validation suite** for the Nova Engine. Unlike synthetic benchmarks, this suite executes real computation workloads and validates results with checksums.

## Strict Requirements Met

✅ **NO synthetic timing shortcuts**  
✅ **NO mocked latency values**  
✅ **ALL workloads execute real computation**  
✅ **ALL benchmarks include checksum validation**  
✅ **ALL measurements prevent dead-code elimination**  
✅ **High-resolution timers** (mach_absolute_time / clock_gettime)  
✅ **Warmup phases mandatory**  
✅ **Multiple iteration averaging mandatory**  

## Benchmark Sections

### Section 1: Kernel-Level Microbenchmarks
- **Matrix Multiplication** (FP32, FP16, INT8)
  - Sizes: 64×64×64, 256×256×256, 1024×1024×1024
  - Variants: Naive Scalar, SIMD Optimized, Register Tiled, Fused
  - Metrics: Latency (μs/ms), GFLOPS/TOPS, Cache sensitivity, Checksum validation

### Section 2: Memory System Stress
- **Access Patterns**: Sequential, Random Pointer Chasing, Strided, Large Copy
- **Metrics**: Bandwidth (GB/s), Latency per access (ns/op), Cache hierarchy detection (L1/L2/L3/DRAM)
- **Protection**: Memory fences, volatile, checksum accumulation

### Section 3: Fusion Efficiency Tests
- **Comparison**: Unfused (MatMul → Add → Activation) vs Fused kernel
- **Metrics**: Latency reduction, Memory traffic reduction, Effective speedup

### Section 4: Numerics Stability Validation
- **Methods**: Standard, Kahan Summation, Pairwise Reduction, Mixed Precision
- **Workloads**: Large reductions, Dot products, Softmax stability
- **Metrics**: ULP drift, Relative error, Determinism consistency

### Section 5: Attention / AI Workloads
- **Variants**: Naive Attention, IO-Aware Tiled, Fused Attention
- **Sequence Lengths**: 128, 512, 2048, 4096
- **Metrics**: Latency, Effective FLOPS, Memory traffic, Numerical drift

### Section 6: Synthetic vs Materialized Data Movement
- **Workloads**: Materialized positional encoding, Synthetic generation, Recurrence
- **Metrics**: Latency, Bandwidth usage, Compute vs memory trade-off

### Section 7: End-to-End Graph Execution
- **Graphs**: Transformer block, MLP block
- **Metrics**: Dispatch overhead, Fusion impact, Throughput (tokens/sec), Determinism

## Anomaly Detection

The suite automatically detects:
- **Impossible bandwidth** (measured > 1.2x theoretical)
- **Suspiciously low latency** (< 0.1 μs, likely dead code elimination)
- **Invalid checksums** (0 or 0xFFFF..., possible dead code)

## Building

### Quick Build
```bash
cd native/src/tools
make -f Makefile.validation
```

### Build Variants
```bash
# Debug build (with symbols, -O0)
make -f Makefile.validation debug

# Release build (aggressive optimizations, -O3 -march=native)
make -f Makefile.validation release

# Clean
make -f Makefile.validation clean
```

## Running

### Run All Benchmarks
```bash
make -f Makefile.validation run
```

### Direct Execution
```bash
./nova_scientific_validation
```

## Output Format

All output follows a scientific/engineering tone with clear markers:

```
[ANALYSIS]          - High-level analysis context
[MEASURED]          - Actual hardware measurements
[VALIDATED]         - Checksum verification status
[CHECKSUM VERIFIED] - Checksum value
[WARNING]           - Anomaly detection warnings
```

### Example Output

```
[ANALYSIS] FP32 Matrix Multiplication (256×256×256)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[MEASURED] Naive Scalar:
  [MEASURED] Latency: 45.234 μs (45.234 ms)
  [MEASURED] Throughput: 187.45 GFLOPS
  [VALIDATED] Checksum: 0x1a2b3c4d5e6f7890 (PASS)
  [MEASURED] Iterations: 20

[MEASURED] SIMD Optimized:
  [MEASURED] Latency: 12.456 μs (12.456 ms)
  [MEASURED] Throughput: 681.23 GFLOPS
  [VALIDATED] Checksum: 0x9f8e7d6c5b4a3210 (PASS)
  [MEASURED] Iterations: 20
[ANALYSIS] SIMD Speedup: 3.63x
```

## Hardware Timing Sources

- **macOS (Apple Silicon/Intel)**: `mach_absolute_time()` with timebase conversion
- **Linux**: `clock_gettime(CLOCK_MONOTONIC)` with nanosecond precision

## Preventing Dead Code Elimination

The suite uses multiple techniques:
1. **Checksum accumulation** - Results XORed into checksums
2. **Volatile declarations** - Memory access patterns with `volatile`
3. **Memory fences** - Explicit ordering constraints
4. **Validation checks** - Anomaly detection for suspicious patterns

## Architecture Support

- **ARM64 (Apple Silicon, ARM servers)**: NEON SIMD intrinsics
- **x86_64**: SSE/AVX support (fallback to scalar)
- **Portable**: Falls back to scalar implementations

## File Structure

```
nova_scientific_validation.h                 - Main header
nova_scientific_validation.c                 - Core infrastructure
nova_scientific_validation_matmul.c          - Section 1: MatMul kernels
nova_scientific_validation_memory.c          - Section 2: Memory stress
nova_scientific_validation_fusion.c          - Section 3: Fusion tests
nova_scientific_validation_numerics.c        - Section 4: Numerics validation
nova_scientific_validation_attention.c       - Section 5: Attention workloads
nova_scientific_validation_datamovement.c    - Section 6: Data movement
nova_scientific_validation_graph.c           - Section 7: Graph execution
nova_scientific_validation_main.c            - Main driver
Makefile.validation                            - Build system
```

## Performance Expectations

Typical run time on Apple M1/M2/M3:
- **Full suite**: ~2-5 minutes
- **Section 1 (MatMul)**: ~30-60 seconds
- **Section 2 (Memory)**: ~20-40 seconds
- **Section 5 (Attention)**: ~40-90 seconds

## Integration with Nova

This suite can be integrated into:
1. **CI/CD pipelines** - Detect performance regressions
2. **Release validation** - Verify performance claims
3. **Hardware profiling** - Understand platform characteristics
4. **Optimization validation** - Measure optimization impact

## Key Differences from Synthetic Benchmarks

| Aspect | Synthetic Benchmarks | This Suite |
|--------|---------------------|------------|
| Timing | Mocked/Simulated | Real hardware timers |
| Computation | May be optimized away | Checksum-validated |
| Validation | Optional | Mandatory |
| Warmup | Often skipped | Always performed |
| Averaging | Single run common | Multiple iterations |
| Anomaly Detection | Rare | Built-in |

## Scientific Rigor

This suite follows scientific benchmarking best practices:
- **Reproducibility**: Multiple iterations, warmup phases
- **Validation**: Checksum verification prevents compiler tricks
- **Transparency**: Clear reporting of all metrics
- **Realism**: Actual hardware execution, no simulation

## License

Same as Nova Engine (see parent LICENSE file)

## Contact

For questions or issues with the validation suite, please file an issue in the main Nova repository.
