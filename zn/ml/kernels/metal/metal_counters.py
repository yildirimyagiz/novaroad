#!/usr/bin/env python3
"""
Metal Performance Counters Measurement
Uses system profiling tools to measure GPU performance metrics
"""

import subprocess
import time
import re
import os
import signal
import threading
from typing import Dict, List, Optional
import tempfile
import json

class MetalPerformanceCounters:
    def __init__(self):
        self.powermetrics_available = self._check_powermetrics()
        self.instruments_available = self._check_instruments()

    def _check_powermetrics(self) -> bool:
        """Check if powermetrics is available (requires sudo)"""
        try:
            result = subprocess.run(['which', 'powermetrics'],
                                  capture_output=True, text=True)
            return result.returncode == 0
        except:
            return False

    def _check_instruments(self) -> bool:
        """Check if Instruments is available"""
        try:
            result = subprocess.run(['which', 'instruments'],
                                  capture_output=True, text=True)
            return result.returncode == 0
        except:
            return False

    def measure_with_powermetrics(self, duration: int = 5) -> Dict[str, float]:
        """Measure GPU performance using powermetrics"""
        if not self.powermetrics_available:
            print("powermetrics not available. Run with sudo for GPU counters.")
            return {}

        cmd = [
            'sudo', 'powermetrics',
            '--samplers', 'gpu_power',
            '--show-process-gpu',
            '--show-process-energy',
            '--show-gpu-stats',
            '-i', str(duration * 1000),  # duration in ms
            '-n', '1'  # number of samples
        ]

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=duration + 2)
            return self._parse_powermetrics_output(result.stdout)
        except subprocess.TimeoutExpired:
            return {}
        except Exception as e:
            print(f"Error running powermetrics: {e}")
            return {}

    def _parse_powermetrics_output(self, output: str) -> Dict[str, float]:
        """Parse powermetrics output for GPU metrics"""
        metrics = {}

        # GPU utilization patterns
        gpu_util_match = re.search(r'GPU active residency:\s*(\d+\.\d+)%', output)
        if gpu_util_match:
            metrics['gpu_utilization_percent'] = float(gpu_util_match.group(1))

        # GPU frequency
        gpu_freq_match = re.search(r'GPU frequency:\s*(\d+)\s*MHz', output)
        if gpu_freq_match:
            metrics['gpu_frequency_mhz'] = float(gpu_freq_match.group(1))

        # Memory bandwidth
        mem_bw_match = re.search(r'Memory bandwidth:\s*(\d+\.\d+)\s*GB/s', output)
        if mem_bw_match:
            metrics['memory_bandwidth_gbps'] = float(mem_bw_match.group(1))

        # Power consumption
        power_match = re.search(r'GPU power:\s*(\d+)\s*mW', output)
        if power_match:
            metrics['gpu_power_mw'] = float(power_match.group(1))

        return metrics

    def profile_kernel_execution(self, kernel_func: callable, *args, **kwargs) -> Dict[str, float]:
        """Profile kernel execution with performance counters"""

        # Start powermetrics in background if available
        powermetrics_thread = None
        if self.powermetrics_available:
            powermetrics_thread = threading.Thread(
                target=self._run_powermetrics_background,
                args=(10,)  # 10 second sampling
            )
            powermetrics_thread.daemon = True
            powermetrics_thread.start()
            time.sleep(1)  # Let powermetrics start

        # Execute kernel
        start_time = time.time()
        result = kernel_func(*args, **kwargs)
        end_time = time.time()

        execution_time = (end_time - start_time) * 1000  # ms

        # Get final powermetrics data
        final_metrics = {}
        if powermetrics_thread and self.powermetrics_available:
            time.sleep(2)  # Let powermetrics finish sampling
            # In a real implementation, you'd capture the background process output
            final_metrics = self.measure_with_powermetrics(1)

        # Calculate derived metrics
        derived_metrics = self._calculate_derived_metrics(execution_time, result)

        return {
            'execution_time_ms': execution_time,
            'powermetrics_data': final_metrics,
            'derived_metrics': derived_metrics,
            'kernel_result': str(type(result))
        }

    def _run_powermetrics_background(self, duration: int):
        """Run powermetrics in background"""
        # This would be implemented to capture data during execution
        pass

    def _calculate_derived_metrics(self, execution_time_ms: float, result) -> Dict[str, float]:
        """Calculate derived performance metrics"""
        metrics = {}

        # Estimate FLOPS based on result size (rough approximation)
        if hasattr(result, 'numel'):
            # Assume tensor operations
            num_elements = result.numel()
            estimated_flops = num_elements * 10  # Rough estimate for attention/matmuls
            metrics['estimated_tflops'] = (estimated_flops / 1e12) / (execution_time_ms / 1000)

        # Memory efficiency (rough estimate)
        if hasattr(result, 'element_size') and hasattr(result, 'numel'):
            memory_bytes = result.numel() * result.element_size()
            memory_accesses = memory_bytes * 3  # Read A, read B, write C
            metrics['estimated_memory_bandwidth_gbps'] = (memory_accesses / 1e9) / (execution_time_ms / 1000)

        return metrics

def benchmark_flash_attention_counters(batch_size: int = 2, num_heads: int = 8,
                                     seq_len: int = 1024, head_dim: int = 64):
    """Benchmark FlashAttention with performance counters"""

    import torch
    device = torch.device('mps' if torch.backends.mps.is_available() else 'cpu')

    def attention_kernel():
        q = torch.randn(batch_size, num_heads, seq_len, head_dim, device=device)
        k = torch.randn(batch_size, num_heads, seq_len, head_dim, device=device)
        v = torch.randn(batch_size, num_heads, seq_len, head_dim, device=device)

        out = torch.nn.functional.scaled_dot_product_attention(q, k, v)
        if torch.backends.mps.is_available():
            torch.mps.synchronize()
        return out

    profiler = MetalPerformanceCounters()
    results = profiler.profile_kernel_execution(attention_kernel)

    print("\n=== Metal Performance Counters - FlashAttention ===")
    print(f"Configuration: batch={batch_size}, heads={num_heads}, seq={seq_len}, dim={head_dim}")
    print(".2f")

    if results.get('powermetrics_data'):
        pm = results['powermetrics_data']
        print("\nPowerMetrics Data:")
        for key, value in pm.items():
            print(".2f")

    if results.get('derived_metrics'):
        dm = results['derived_metrics']
        print("\nDerived Metrics:")
        for key, value in dm.items():
            if 'tflops' in key.lower():
                print(".2f")
            elif 'bandwidth' in key.lower():
                print(".2f")
            else:
                print(".3f")

    return results

def benchmark_matmul_counters(size: int = 1024):
    """Benchmark matrix multiplication with performance counters"""

    import torch
    device = torch.device('mps' if torch.backends.mps.is_available() else 'cpu')

    def matmul_kernel():
        a = torch.randn(size, size, device=device)
        b = torch.randn(size, size, device=device)
        c = torch.mm(a, b)
        if torch.backends.mps.is_available():
            torch.mps.synchronize()
        return c

    profiler = MetalPerformanceCounters()
    results = profiler.profile_kernel_execution(matmul_kernel)

    print("\n=== Metal Performance Counters - MatMul ===")
    print(f"Matrix size: {size}x{size}")
    print(".2f")

    if results.get('powermetrics_data'):
        pm = results['powermetrics_data']
        print("\nPowerMetrics Data:")
        for key, value in pm.items():
            print(".2f")

    if results.get('derived_metrics'):
        dm = results['derived_metrics']
        print("\nDerived Metrics:")
        for key, value in dm.items():
            if 'tflops' in key.lower():
                print(".2f")
            elif 'bandwidth' in key.lower():
                print(".2f")
            else:
                print(".3f")

    return results

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Metal Performance Counters')
    parser.add_argument('--kernel', choices=['flash_attention', 'matmul'], default='flash_attention')
    parser.add_argument('--batch', type=int, default=2)
    parser.add_argument('--heads', type=int, default=8)
    parser.add_argument('--seq', type=int, default=1024)
    parser.add_argument('--dim', type=int, default=64)
    parser.add_argument('--size', type=int, default=1024)

    args = parser.parse_args()

    if args.kernel == 'flash_attention':
        benchmark_flash_attention_counters(args.batch, args.heads, args.seq, args.dim)
    elif args.kernel == 'matmul':
        benchmark_matmul_counters(args.size)
