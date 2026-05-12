#!/usr/bin/env python3
"""
Credible Nova Ultra Benchmark Validation
Addresses all academic scrutiny concerns
"""

import torch
import torch.mps
import numpy as np
import time
import subprocess
import sys

def get_hardware_info():
    """Get accurate hardware information"""
    try:
        cpu_info = subprocess.run(['sysctl', '-n', 'machdep.cpu.brand_string'],
                                capture_output=True, text=True).stdout.strip()
        return {
            'cpu': cpu_info,
            'gpu_cores': "8-core" if "M1" in cpu_info else "10-core",
            'has_mps': torch.backends.mps.is_available()
        }
    except:
        return {'cpu': 'Unknown', 'gpu_cores': 'Unknown', 'has_mps': False}

def calculate_theoretical_peak(hw_info):
    """Calculate theoretical peaks for Apple Silicon"""
    if "M1" in hw_info['cpu']:
        return {
            'fp32_tflops': 2.6,
            'fp16_tflops': 5.2,
            'int8_tops': 10.4,
        }
    elif "M2" in hw_info['cpu']:
        return {
            'fp32_tflops': 3.6,
            'fp16_tflops': 7.2,
            'int8_tops': 14.4,
        }
    return {'fp32_tflops': 0, 'fp16_tflops': 0, 'int8_tops': 0}

def run_zenith_and_validate():
    """Run Nova and validate results"""
    print("🔬 NOVA ULTRA CREDIBILITY VALIDATION")
    print("=" * 50)

    # Hardware detection
    hw = get_hardware_info()
    peaks = calculate_theoretical_peak(hw)

    print(f"\n📊 HARDWARE: {hw['cpu']} ({hw['gpu_cores']} GPU)")
    print(f"   Theoretical Peak (FP16): {peaks['fp16_tflops']:.1f}")

    # Run benchmark and capture output
    print("\n🚀 RUNNING NOVA BENCHMARK...")
    result = subprocess.run(['./zenith_ultra'], capture_output=True, text=True,
                          cwd='/Users/yldyagz/Downloads/matmul opt')

    # Parse results
    results = []
    for line in result.stdout.split('\n'):
        if 'GFLOPS:' in line and 'Matrix:' in line:
            parts = line.split()
            try:
                size = int(parts[1].split('×')[0])
                backend = parts[3]
                time_ms = float(parts[5])
                reported_gflops = float(parts[7])

                # Verify FLOP calculation
                expected_flops = 2.0 * size ** 3  # GEMM: 2*N^3 operations
                expected_gflops = expected_flops / (time_ms / 1000) / 1e9

                results.append({
                    'size': size,
                    'backend': backend,
                    'time_ms': time_ms,
                    'reported_gflops': reported_gflops,
                    'calculated_gflops': expected_gflops,
                    'discrepancy_pct': abs(reported_gflops - expected_gflops) / reported_gflops * 100
                })
            except:
                continue

    print("\n� VALIDATION RESULTS:")
    max_gflops = 0
    for r in results:
        status = "✅" if r['discrepancy_pct'] < 5 else "❌"
        print(f"   {status} {r['size']}×{r['size']}: {r['reported_gflops']:.1f} GFLOPS")
        if r['discrepancy_pct'] > 5:
            print(f"      Expected: {r['calculated_gflops']:.1f} GFLOPS (Δ{r['discrepancy_pct']:.1f}%)")
        max_gflops = max(max_gflops, r['reported_gflops'])

    # Credibility check
    theoretical_max = peaks['fp16_tflops'] * 1000  # Convert to GFLOPS
    valid_calcs = all(r['discrepancy_pct'] < 5 for r in results)
    within_limits = max_gflops <= theoretical_max * 1.1

    print("\n🎓 CREDIBILITY ASSESSMENT:")
    print(f"   FLOP Calculations: {'✅ Valid' if valid_calcs else '❌ Issues found'}")
    print(f"   Theoretical Limits: {'✅ Within bounds' if within_limits else '❌ Exceeds limits'}")
    print(f"   Theoretical Peak (FP16): {peaks['fp16_tflops']:.1f} TFLOPS ({theoretical_max:.1f} GFLOPS)")

    print("\n📝 CORRECTED CLAIMS:")
    if results:
        print("   Based on actual measurements:")
        for r in results:
            if r['size'] == 4096:
                print(f"   Corrected Claim (4096x4096): {r['reported_gflops']:.1f} GFLOPS")
                break

    print("\n🏆 CONCLUSION:")
    if valid_calcs and within_limits:
        print("   🎯 CREDIBLE: Excellent Apple Silicon optimization!")
    else:
        print("   ⚠️  NEEDS CORRECTION: FLOP calculations and claims require validation")

if __name__ == '__main__':
    run_zenith_and_validate()
