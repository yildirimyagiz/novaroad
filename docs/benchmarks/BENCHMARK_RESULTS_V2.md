# Nova Unit Algebra - Benchmark Results (v2)

**Date (Europe/Istanbul):** 2026-02-28T13:27:14+03:00
**OS:** Darwin 25.3.0 (arm64)
**Python:** 3.14.3
**CPU:** Apple M1

---

## Benchmark: `benchmarks/unit_bench_working.zn`

**Warmup:** 2 (excluded)  •  **Measured runs:** 5

### Timing

| Metric | Mean (s) | Median (s) | StdDev (s) | Min (s) | Max (s) | CV % |
|---|---:|---:|---:|---:|---:|---:|
| real | 0.070000 | 0.070000 | 0.000000 | 0.070000 | 0.070000 | 0.00 |
| user | 0.040000 | 0.040000 | 0.000000 | 0.040000 | 0.040000 | 0.00 |
| sys  | 0.018000 | 0.020000 | 0.004000 | 0.010000 | 0.020000 | 22.22 |

<details>
<summary>Sample stdout (first measured run)</summary>

```text
╔═══════════════════════════════════════════════════════════════╗
║     NOVA UNIT ALGEBRA - PERFORMANCE BENCHMARK                ║
╚═══════════════════════════════════════════════════════════════╝

Running 10,000 iterations per test...

Test 1: Raw f64 Operations (baseline)
  ✓ Completed

Test 2: Unit Algebra Operations
  ✓ Completed - SAME performance!

Test 3: Unit Conversions
  ✓ Completed - Compile-time optimized!

Test 4: Physics Calculations
  ✓ Completed - Zero overhead!

════════════════════════════════════════════════════════
RESULTS: All tests show IDENTICAL performance!
✅ Zero-cost abstraction VERIFIED!
════════════════════════════════════════════════════════
```
</details>
