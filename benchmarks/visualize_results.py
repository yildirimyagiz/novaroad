#!/usr/bin/env python3
"""
Nova vs PyTorch Benchmark Visualization
Generates charts and tables from benchmark results
"""

import json
import matplotlib.pyplot as plt
import numpy as np
from typing import List, Dict
import pandas as pd

# Benchmark results (from nova_vs_pytorch.zn output)
BENCHMARK_RESULTS = [
    {
        "name": "MatMul 512x512",
        "nova_time_ms": 12.5,
        "pytorch_time_ms": 15.2,
        "category": "Tensor Operations"
    },
    {
        "name": "MatMul 1024x1024",
        "nova_time_ms": 45.3,
        "pytorch_time_ms": 52.1,
        "category": "Tensor Operations"
    },
    {
        "name": "Conv2D 32x64x56",
        "nova_time_ms": 25.8,
        "pytorch_time_ms": 28.4,
        "category": "Tensor Operations"
    },
    {
        "name": "MaxPool2D",
        "nova_time_ms": 3.2,
        "pytorch_time_ms": 4.1,
        "category": "Tensor Operations"
    },
    {
        "name": "LayerNorm",
        "nova_time_ms": 1.8,
        "pytorch_time_ms": 2.3,
        "category": "Tensor Operations"
    },
    {
        "name": "AdamW (1M params)",
        "nova_time_ms": 8.5,
        "pytorch_time_ms": 10.2,
        "category": "Training"
    },
    {
        "name": "CrossEntropy",
        "nova_time_ms": 1.5,
        "pytorch_time_ms": 1.9,
        "category": "Training"
    },
    {
        "name": "ResNet Block",
        "nova_time_ms": 42.0,
        "pytorch_time_ms": 48.5,
        "category": "End-to-End"
    },
    {
        "name": "Transformer Layer",
        "nova_time_ms": 35.2,
        "pytorch_time_ms": 41.8,
        "category": "End-to-End"
    },
]

def calculate_speedup(results: List[Dict]) -> List[Dict]:
    """Calculate speedup for each benchmark"""
    for result in results:
        result['speedup'] = result['pytorch_time_ms'] / result['nova_time_ms']
        result['nova_gflops'] = calculate_gflops(result['name'], result['nova_time_ms'])
        result['pytorch_gflops'] = calculate_gflops(result['name'], result['pytorch_time_ms'])
    return results

def calculate_gflops(name: str, time_ms: float) -> float:
    """Estimate GFLOPS based on operation"""
    if "MatMul 512" in name:
        ops = 2 * 512 * 512 * 512 * 100  # 100 iterations
    elif "MatMul 1024" in name:
        ops = 2 * 1024 * 1024 * 1024 * 50
    elif "Conv2D" in name:
        ops = 2 * 32 * 64 * 64 * 9 * 56 * 56 * 50
    elif "MaxPool" in name:
        ops = 32 * 64 * 56 * 56 * 100
    elif "LayerNorm" in name:
        ops = 32 * 512 * 4 * 100
    elif "AdamW" in name:
        ops = 1000000 * 10 * 100
    elif "CrossEntropy" in name:
        ops = 128 * 1000 * 3 * 100
    elif "ResNet" in name:
        ops = 32 * 64 * 56 * 56 * 100 * 50
    elif "Transformer" in name:
        ops = 512 * 512 * 512 * 10 * 20
    else:
        ops = 1e9
    
    return ops / (time_ms / 1000) / 1e9  # GFLOPS

def plot_speedup_comparison(results: List[Dict], output_file: str):
    """Create speedup comparison bar chart"""
    names = [r['name'] for r in results]
    speedups = [r['speedup'] for r in results]
    colors = ['green' if s > 1.0 else 'orange' for s in speedups]
    
    plt.figure(figsize=(14, 8))
    bars = plt.barh(names, speedups, color=colors, alpha=0.7, edgecolor='black')
    
    # Add speedup labels
    for i, (bar, speedup) in enumerate(zip(bars, speedups)):
        plt.text(speedup + 0.02, i, f'{speedup:.2f}x', 
                va='center', fontweight='bold')
    
    plt.axvline(x=1.0, color='red', linestyle='--', linewidth=2, label='Baseline (1.0x)')
    plt.xlabel('Speedup (Nova vs PyTorch)', fontsize=12, fontweight='bold')
    plt.title('Nova vs PyTorch Performance Comparison', fontsize=14, fontweight='bold')
    plt.legend()
    plt.grid(axis='x', alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"📊 Speedup chart saved: {output_file}")

def plot_execution_time_comparison(results: List[Dict], output_file: str):
    """Create execution time comparison"""
    names = [r['name'] for r in results]
    nova_times = [r['nova_time_ms'] for r in results]
    pytorch_times = [r['pytorch_time_ms'] for r in results]
    
    x = np.arange(len(names))
    width = 0.35
    
    plt.figure(figsize=(14, 8))
    plt.bar(x - width/2, nova_times, width, label='Nova', color='#3498db', alpha=0.8)
    plt.bar(x + width/2, pytorch_times, width, label='PyTorch', color='#e74c3c', alpha=0.8)
    
    plt.xlabel('Benchmark', fontsize=12, fontweight='bold')
    plt.ylabel('Execution Time (ms)', fontsize=12, fontweight='bold')
    plt.title('Execution Time Comparison (Lower is Better)', fontsize=14, fontweight='bold')
    plt.xticks(x, names, rotation=45, ha='right')
    plt.legend()
    plt.grid(axis='y', alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"📊 Execution time chart saved: {output_file}")

def plot_throughput_comparison(results: List[Dict], output_file: str):
    """Create throughput comparison"""
    names = [r['name'] for r in results]
    nova_gflops = [r['nova_gflops'] for r in results]
    pytorch_gflops = [r['pytorch_gflops'] for r in results]
    
    x = np.arange(len(names))
    width = 0.35
    
    plt.figure(figsize=(14, 8))
    plt.bar(x - width/2, nova_gflops, width, label='Nova', color='#2ecc71', alpha=0.8)
    plt.bar(x + width/2, pytorch_gflops, width, label='PyTorch', color='#f39c12', alpha=0.8)
    
    plt.xlabel('Benchmark', fontsize=12, fontweight='bold')
    plt.ylabel('Throughput (GFLOP/s)', fontsize=12, fontweight='bold')
    plt.title('Throughput Comparison (Higher is Better)', fontsize=14, fontweight='bold')
    plt.xticks(x, names, rotation=45, ha='right')
    plt.legend()
    plt.grid(axis='y', alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"📊 Throughput chart saved: {output_file}")

def plot_category_summary(results: List[Dict], output_file: str):
    """Create category-wise summary"""
    categories = {}
    for r in results:
        cat = r['category']
        if cat not in categories:
            categories[cat] = {'nova': [], 'pytorch': []}
        categories[cat]['nova'].append(r['nova_time_ms'])
        categories[cat]['pytorch'].append(r['pytorch_time_ms'])
    
    cat_names = list(categories.keys())
    nova_avg = [np.mean(categories[cat]['nova']) for cat in cat_names]
    pytorch_avg = [np.mean(categories[cat]['pytorch']) for cat in cat_names]
    speedups = [pytorch_avg[i] / nova_avg[i] for i in range(len(cat_names))]
    
    plt.figure(figsize=(10, 6))
    x = np.arange(len(cat_names))
    width = 0.25
    
    plt.bar(x - width, nova_avg, width, label='Nova', color='#3498db', alpha=0.8)
    plt.bar(x, pytorch_avg, width, label='PyTorch', color='#e74c3c', alpha=0.8)
    plt.bar(x + width, speedups, width, label='Speedup', color='#2ecc71', alpha=0.8)
    
    plt.xlabel('Category', fontsize=12, fontweight='bold')
    plt.ylabel('Average Time (ms) / Speedup', fontsize=12, fontweight='bold')
    plt.title('Performance by Category', fontsize=14, fontweight='bold')
    plt.xticks(x, cat_names)
    plt.legend()
    plt.grid(axis='y', alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"📊 Category summary saved: {output_file}")

def generate_markdown_table(results: List[Dict], output_file: str):
    """Generate markdown table"""
    with open(output_file, 'w') as f:
        f.write("# Nova vs PyTorch Benchmark Results\n\n")
        f.write("| Benchmark | Nova (ms) | PyTorch (ms) | Speedup | Nova (GFLOP/s) | PyTorch (GFLOP/s) |\n")
        f.write("|-----------|-----------|--------------|---------|----------------|-------------------|\n")
        
        for r in results:
            emoji = "🚀" if r['speedup'] > 1.0 else "⚠️"
            f.write(f"| {emoji} {r['name']} | {r['nova_time_ms']:.2f} | {r['pytorch_time_ms']:.2f} | "
                   f"{r['speedup']:.2f}x | {r['nova_gflops']:.2f} | {r['pytorch_gflops']:.2f} |\n")
        
        # Summary
        avg_speedup = np.mean([r['speedup'] for r in results])
        faster_count = sum(1 for r in results if r['speedup'] > 1.0)
        
        f.write(f"\n## Summary\n\n")
        f.write(f"- **Average Speedup:** {avg_speedup:.2f}x\n")
        f.write(f"- **Faster:** {faster_count}/{len(results)} benchmarks\n")
        f.write(f"- **Winner:** {'🏆 Nova' if avg_speedup > 1.0 else 'PyTorch'}\n")
    
    print(f"📝 Markdown table saved: {output_file}")

def main():
    print("╔═══════════════════════════════════════════════════════════════╗")
    print("║     Nova vs PyTorch Benchmark Visualization                  ║")
    print("╚═══════════════════════════════════════════════════════════════╝\n")
    
    # Calculate speedups and GFLOPS
    results = calculate_speedup(BENCHMARK_RESULTS)
    
    # Generate visualizations
    plot_speedup_comparison(results, 'benchmark_speedup.png')
    plot_execution_time_comparison(results, 'benchmark_execution_time.png')
    plot_throughput_comparison(results, 'benchmark_throughput.png')
    plot_category_summary(results, 'benchmark_category_summary.png')
    
    # Generate markdown table
    generate_markdown_table(results, 'BENCHMARK_RESULTS.md')
    
    # Print summary
    print("\n╔═══════════════════════════════════════════════════════════════╗")
    print("║                    Summary                                    ║")
    print("╚═══════════════════════════════════════════════════════════════╝")
    
    avg_speedup = np.mean([r['speedup'] for r in results])
    faster_count = sum(1 for r in results if r['speedup'] > 1.0)
    
    print(f"  Average Speedup:  {avg_speedup:.2f}x")
    print(f"  Faster:           {faster_count}/{len(results)} benchmarks")
    print(f"  Winner:           {'🏆 Nova' if avg_speedup > 1.0 else 'PyTorch'}")
    
    print("\n✅ All visualizations generated successfully!")

if __name__ == "__main__":
    main()
