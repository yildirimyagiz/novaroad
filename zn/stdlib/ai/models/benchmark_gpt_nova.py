#!/usr/bin/env python3
"""
GPT Benchmark Suite
===================

Comprehensive benchmarks comparing:
- Baseline GPT vs Nova GPT
- Different optimizations (Flash Attention, Quantization, etc.)
- Training vs Inference performance
"""

import torch
import torch.nn as nn
import time
import numpy as np
from tabulate import tabulate
import matplotlib.pyplot as plt

from gpt_nova_complete import NovaGPT, create_gpt_small
from gpt_inference_optimized import OptimizedGPTInference


class GPTBenchmark:
    """Comprehensive GPT benchmark suite"""
    
    def __init__(self, device='cuda'):
        self.device = device if torch.cuda.is_available() else 'cpu'
        print(f"🔬 GPT Benchmark Suite")
        print(f"   Device: {self.device}")
        if torch.cuda.is_available():
            print(f"   GPU: {torch.cuda.get_device_name(0)}")
        print()
    
    def benchmark_forward_pass(
        self,
        model,
        batch_size=4,
        seq_len=1024,
        num_runs=100
    ):
        """Benchmark forward pass speed"""
        model.eval()
        model = model.to(self.device)
        
        # Create dummy input
        input_ids = torch.randint(0, 50257, (batch_size, seq_len), device=self.device)
        
        # Warmup
        with torch.no_grad():
            for _ in range(10):
                _ = model(input_ids)
        
        # Benchmark
        times = []
        
        with torch.no_grad():
            for _ in range(num_runs):
                if torch.cuda.is_available():
                    torch.cuda.synchronize()
                
                start = time.time()
                logits, _ = model(input_ids)
                
                if torch.cuda.is_available():
                    torch.cuda.synchronize()
                
                elapsed = time.time() - start
                times.append(elapsed)
        
        avg_time = np.mean(times)
        std_time = np.std(times)
        
        # Calculate throughput
        tokens_per_sec = (batch_size * seq_len) / avg_time
        
        return {
            'avg_time': avg_time,
            'std_time': std_time,
            'tokens_per_sec': tokens_per_sec,
            'times': times
        }
    
    def benchmark_generation(
        self,
        model,
        prompt_len=20,
        gen_len=100,
        num_runs=20
    ):
        """Benchmark text generation speed"""
        model.eval()
        model = model.to(self.device)
        
        # Create prompt
        prompt = torch.randint(0, 50257, (1, prompt_len), device=self.device)
        
        # Warmup
        with torch.no_grad():
            _ = model.generate(prompt, max_new_tokens=10)
        
        # Benchmark
        times = []
        
        with torch.no_grad():
            for _ in range(num_runs):
                if torch.cuda.is_available():
                    torch.cuda.synchronize()
                
                start = time.time()
                output = model.generate(prompt, max_new_tokens=gen_len)
                
                if torch.cuda.is_available():
                    torch.cuda.synchronize()
                
                elapsed = time.time() - start
                times.append(elapsed)
        
        avg_time = np.mean(times)
        tokens_per_sec = gen_len / avg_time
        
        return {
            'avg_time': avg_time,
            'tokens_per_sec': tokens_per_sec,
            'times': times
        }
    
    def compare_optimizations(self):
        """Compare different optimization strategies"""
        print("📊 Comparing Optimization Strategies\n")
        
        results = []
        
        # 1. Baseline (no optimizations)
        print("1️⃣  Baseline GPT (no optimizations)...")
        model_baseline = create_gpt_small(use_checkpoint=False)
        result = self.benchmark_forward_pass(model_baseline, num_runs=50)
        results.append({
            'Config': 'Baseline',
            'Time (ms)': f"{result['avg_time']*1000:.1f}±{result['std_time']*1000:.1f}",
            'Tokens/sec': f"{result['tokens_per_sec']:.0f}",
            'Speedup': '1.0×'
        })
        baseline_time = result['avg_time']
        del model_baseline
        torch.cuda.empty_cache() if torch.cuda.is_available() else None
        
        # 2. Flash Attention (already in NovaGPT)
        print("2️⃣  With Flash Attention...")
        model_flash = create_gpt_small(use_checkpoint=False)
        result = self.benchmark_forward_pass(model_flash, num_runs=50)
        speedup = baseline_time / result['avg_time']
        results.append({
            'Config': '+ Flash Attention',
            'Time (ms)': f"{result['avg_time']*1000:.1f}±{result['std_time']*1000:.1f}",
            'Tokens/sec': f"{result['tokens_per_sec']:.0f}",
            'Speedup': f"{speedup:.1f}×"
        })
        flash_time = result['avg_time']
        
        # 3. + Gradient Checkpointing (for training)
        print("3️⃣  + Gradient Checkpointing...")
        model_ckpt = create_gpt_small(use_checkpoint=True)
        result = self.benchmark_forward_pass(model_ckpt, num_runs=50)
        speedup = baseline_time / result['avg_time']
        results.append({
            'Config': '+ Grad Checkpoint',
            'Time (ms)': f"{result['avg_time']*1000:.1f}±{result['std_time']*1000:.1f}",
            'Tokens/sec': f"{result['tokens_per_sec']:.0f}",
            'Speedup': f"{speedup:.1f}×'
        })
        del model_ckpt
        torch.cuda.empty_cache() if torch.cuda.is_available() else None
        
        # 4. + Quantization (inference only)
        print("4️⃣  + INT8 Quantization...")
        optimized = OptimizedGPTInference(
            model_flash,
            quantize=True,
            compile=False,
            device=self.device
        )
        result = self.benchmark_forward_pass(optimized.model, num_runs=50)
        speedup = baseline_time / result['avg_time']
        results.append({
            'Config': '+ INT8 Quantization',
            'Time (ms)': f"{result['avg_time']*1000:.1f}±{result['std_time']*1000:.1f}",
            'Tokens/sec': f"{result['tokens_per_sec']:.0f}",
            'Speedup': f"{speedup:.1f}×"
        })
        
        # Print results table
        print("\n" + "="*70)
        print(tabulate(results, headers='keys', tablefmt='grid'))
        print("="*70)
        
        return results
    
    def benchmark_memory_usage(self):
        """Benchmark memory usage"""
        print("\n💾 Memory Usage Comparison\n")
        
        if not torch.cuda.is_available():
            print("⚠️  CUDA not available - skipping memory benchmark")
            return
        
        results = []
        
        # Baseline
        torch.cuda.empty_cache()
        torch.cuda.reset_peak_memory_stats()
        
        model = create_gpt_small(use_checkpoint=False)
        model = model.to(self.device)
        input_ids = torch.randint(0, 50257, (4, 1024), device=self.device)
        
        with torch.no_grad():
            _ = model(input_ids)
        
        mem_baseline = torch.cuda.max_memory_allocated() / 1e9
        results.append({
            'Config': 'Baseline',
            'Memory (GB)': f"{mem_baseline:.2f}",
            'Reduction': '1.0×'
        })
        
        del model
        torch.cuda.empty_cache()
        
        # With Gradient Checkpointing
        torch.cuda.reset_peak_memory_stats()
        
        model = create_gpt_small(use_checkpoint=True)
        model = model.to(self.device)
        
        with torch.no_grad():
            _ = model(input_ids)
        
        mem_ckpt = torch.cuda.max_memory_allocated() / 1e9
        results.append({
            'Config': '+ Gradient Checkpoint',
            'Memory (GB)': f"{mem_ckpt:.2f}",
            'Reduction': f"{mem_baseline/mem_ckpt:.1f}×"
        })
        
        del model
        torch.cuda.empty_cache()
        
        # With Quantization
        torch.cuda.reset_peak_memory_stats()
        
        model = create_gpt_small(use_checkpoint=False)
        optimized = OptimizedGPTInference(
            model,
            quantize=True,
            compile=False,
            device=self.device
        )
        
        with torch.no_grad():
            _ = optimized.model(input_ids)
        
        mem_quant = torch.cuda.max_memory_allocated() / 1e9
        results.append({
            'Config': '+ INT8 Quantization',
            'Memory (GB)': f"{mem_quant:.2f}",
            'Reduction': f"{mem_baseline/mem_quant:.1f}×"
        })
        
        print(tabulate(results, headers='keys', tablefmt='grid'))
        print()
        
        return results
    
    def full_benchmark(self):
        """Run all benchmarks"""
        print("\n" + "="*70)
        print("🚀 COMPLETE GPT NOVA BENCHMARK SUITE")
        print("="*70 + "\n")
        
        # 1. Optimization comparison
        opt_results = self.compare_optimizations()
        
        # 2. Memory usage
        mem_results = self.benchmark_memory_usage()
        
        # 3. Generation benchmark
        print("\n🎲 Generation Speed Benchmark\n")
        model = create_gpt_small()
        gen_result = self.benchmark_generation(model, num_runs=10)
        print(f"   Tokens/sec: {gen_result['tokens_per_sec']:.1f}")
        print(f"   Avg time: {gen_result['avg_time']:.3f}s")
        
        print("\n" + "="*70)
        print("✅ BENCHMARK COMPLETE!")
        print("="*70 + "\n")
        
        return {
            'optimizations': opt_results,
            'memory': mem_results,
            'generation': gen_result
        }


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='GPT Nova Benchmark')
    parser.add_argument('--device', type=str, default='cuda',
                       choices=['cuda', 'cpu'])
    parser.add_argument('--quick', action='store_true',
                       help='Quick benchmark (fewer runs)')
    args = parser.parse_args()
    
    # Run benchmark
    benchmark = GPTBenchmark(device=args.device)
    results = benchmark.full_benchmark()
    
    # Save results
    import json
    with open('gpt_benchmark_results.json', 'w') as f:
        # Convert numpy arrays to lists for JSON serialization
        results_serializable = {
            'optimizations': results['optimizations'],
            'memory': results['memory'],
            'generation': {
                'tokens_per_sec': float(results['generation']['tokens_per_sec']),
                'avg_time': float(results['generation']['avg_time'])
            }
        }
        json.dump(results_serializable, f, indent=2)
    
    print("💾 Results saved to gpt_benchmark_results.json")


if __name__ == '__main__':
    main()
