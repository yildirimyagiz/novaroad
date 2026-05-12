#!/usr/bin/env python3
"""
Memory Usage Profiler for Metal Kernels
Tracks peak memory usage during kernel execution
"""

import torch
import torch.mps
import psutil
import os
import time
import numpy as np
from typing import Dict, List, Callable
import gc

class MemoryProfiler:
    def __init__(self):
        self.device = torch.device('mps' if torch.backends.mps.is_available() else 'cpu')
        self.process = psutil.Process(os.getpid())

    def get_memory_stats(self) -> Dict[str, float]:
        """Get current memory statistics"""
        memory_info = self.process.memory_info()

        stats = {
            'rss_mb': memory_info.rss / 1024 / 1024,
            'vms_mb': memory_info.vms / 1024 / 1024,
            'cpu_percent': self.process.cpu_percent(),
        }

        if torch.backends.mps.is_available():
            try:
                stats['mps_allocated_mb'] = torch.mps.current_allocated_memory() / 1024 / 1024
                stats['mps_reserved_mb'] = torch.mps.driver_allocated_memory() / 1024 / 1024
            except:
                stats['mps_allocated_mb'] = 0.0
                stats['mps_reserved_mb'] = 0.0

        return stats

    def profile_memory_usage(self, func: Callable, *args, **kwargs) -> Dict[str, float]:
        """Profile memory usage during function execution"""

        # Force garbage collection
        gc.collect()

        if torch.backends.mps.is_available():
            torch.mps.empty_cache()
            torch.mps.synchronize()

        # Get baseline memory
        baseline_stats = self.get_memory_stats()
        time.sleep(0.1)  # Small delay for stable readings

        # Track peak memory during execution
        peak_stats = baseline_stats.copy()
        start_time = time.time()

        # Execute function with memory tracking
        try:
            result = func(*args, **kwargs)

            if torch.backends.mps.is_available():
                torch.mps.synchronize()

            # Sample memory multiple times during execution
            for _ in range(10):
                current_stats = self.get_memory_stats()
                for key in peak_stats:
                    if key in current_stats:
                        peak_stats[key] = max(peak_stats[key], current_stats[key])
                time.sleep(0.001)  # 1ms intervals

        except Exception as e:
            print(f"Error during profiling: {e}")
            return {}

        end_time = time.time()
        execution_time = end_time - start_time

        # Calculate deltas
        memory_deltas = {}
        for key in peak_stats:
            if key in baseline_stats:
                memory_deltas[f'{key}_delta'] = peak_stats[key] - baseline_stats[key]
            else:
                memory_deltas[f'{key}_peak'] = peak_stats[key]

        return {
            'execution_time_ms': execution_time * 1000,
            'baseline_stats': baseline_stats,
            'peak_stats': peak_stats,
            'memory_deltas': memory_deltas,
            'result': result
        }

def profile_flash_attention_memory(batch_size: int = 2, num_heads: int = 8,
                                 seq_len: int = 1024, head_dim: int = 64):
    """Profile memory usage for FlashAttention"""

    device = torch.device('mps' if torch.backends.mps.is_available() else 'cpu')

    def attention_func():
        q = torch.randn(batch_size, num_heads, seq_len, head_dim, device=device)
        k = torch.randn(batch_size, num_heads, seq_len, head_dim, device=device)
        v = torch.randn(batch_size, num_heads, seq_len, head_dim, device=device)

        # Standard attention memory usage
        out = torch.nn.functional.scaled_dot_product_attention(q, k, v)
        return out

    profiler = MemoryProfiler()
    results = profiler.profile_memory_usage(attention_func)

    # Calculate theoretical memory usage
    qkv_memory = batch_size * num_heads * seq_len * head_dim * 4 * 3  # Q, K, V (float32)
    attention_matrix_memory = batch_size * num_heads * seq_len * seq_len * 4  # Attention weights
    output_memory = batch_size * num_heads * seq_len * head_dim * 4  # Output

    theoretical_standard = (qkv_memory + attention_matrix_memory + output_memory) / 1024 / 1024  # MB
    theoretical_flash = (qkv_memory + output_memory + batch_size * num_heads * seq_len * 4 * 2) / 1024 / 1024  # MB (L, M stats)

    print("
=== FlashAttention Memory Profile ===")
    print(f"Configuration: batch={batch_size}, heads={num_heads}, seq={seq_len}, dim={head_dim}")
    print(".2f")
    print(".1f")
    print(".1f")
    print(".1f")
    print(".1f")
    print(".1f")

    return results

def profile_matmul_memory(size: int = 1024):
    """Profile memory usage for matrix multiplication"""

    device = torch.device('mps' if torch.backends.mps.is_available() else 'cpu')

    def matmul_func():
        a = torch.randn(size, size, device=device)
        b = torch.randn(size, size, device=device)
        c = torch.mm(a, b)
        return c

    profiler = MemoryProfiler()
    results = profiler.profile_memory_usage(matmul_func)

    # Theoretical memory: 3 matrices of size×size×4 bytes
    theoretical_memory = 3 * size * size * 4 / 1024 / 1024  # MB

    print("
=== MatMul Memory Profile ===")
    print(f"Matrix size: {size}x{size}")
    print(".2f")
    print(".1f")
    print(".1f")
    print(".1f")
    print(".1f")
    print(".1f")

    return results

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Memory Usage Profiler')
    parser.add_argument('--kernel', choices=['flash_attention', 'matmul'], default='flash_attention')
    parser.add_argument('--batch', type=int, default=2)
    parser.add_argument('--heads', type=int, default=8)
    parser.add_argument('--seq', type=int, default=1024)
    parser.add_argument('--dim', type=int, default=64)
    parser.add_argument('--size', type=int, default=1024)

    args = parser.parse_args()

    if args.kernel == 'flash_attention':
        profile_flash_attention_memory(args.batch, args.heads, args.seq, args.dim)
    elif args.kernel == 'matmul':
        profile_matmul_memory(args.size)
