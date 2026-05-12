#!/usr/bin/env python3
"""Text-based visualization of benchmark results"""

import json

def create_bar(value, max_value, width=50):
    """Create ASCII bar chart"""
    filled = int((value / max_value) * width)
    return '█' * filled + '░' * (width - filled)

def main():
    with open('benchmark_results.json', 'r') as f:
        results = json.load(f)
    
    print("╔" + "=" * 78 + "╗")
    print("║" + " " * 20 + "NOVA vs PYTORCH - VISUAL RESULTS" + " " * 26 + "║")
    print("╚" + "=" * 78 + "╝")
    print()
    
    # 1. Speedup Chart
    print("📊 SPEEDUP COMPARISON")
    print("-" * 80)
    
    flash_speedup = results['flash_attention']['speedup']
    quant_speedup = results['quantized_inference']['nova_speedup']
    
    max_speedup = max(flash_speedup, quant_speedup)
    
    print(f"Flash Attention (8× target):")
    print(f"  {create_bar(flash_speedup, max_speedup)} {flash_speedup:.1f}×")
    print()
    print(f"Quantized Inference (30× target):")
    print(f"  {create_bar(quant_speedup, max_speedup)} {quant_speedup:.1f}×")
    print()
    
    # 2. Memory Savings
    print("💾 MEMORY REDUCTION (7B Model)")
    print("-" * 80)
    
    fp32_mem = results['memory_usage']['fp32_gb']
    int8_mem = results['memory_usage']['int8_gb']
    int4_mem = results['memory_usage']['int4_gb']
    
    print(f"PyTorch FP32:  {create_bar(fp32_mem, fp32_mem, 40)} {fp32_mem:.1f} GB")
    print(f"Nova INT8:     {create_bar(int8_mem, fp32_mem, 40)} {int8_mem:.1f} GB ({fp32_mem/int8_mem:.1f}× less)")
    print(f"Nova INT4:     {create_bar(int4_mem, fp32_mem, 40)} {int4_mem:.1f} GB ({fp32_mem/int4_mem:.1f}× less)")
    print()
    
    # 3. Multi-GPU Efficiency
    print("🔄 MULTI-GPU SCALING EFFICIENCY")
    print("-" * 80)
    
    pt_eff = results['multi_gpu']['pytorch_efficiency']
    nova_eff = results['multi_gpu']['nova_efficiency']
    
    print(f"PyTorch DDP:   {create_bar(pt_eff, 100, 40)} {pt_eff:.0f}%")
    print(f"Nova Parallel: {create_bar(nova_eff, 100, 40)} {nova_eff:.0f}%")
    print()
    
    # 4. Summary
    print("✅ TEST RESULTS SUMMARY")
    print("-" * 80)
    
    tests = [
        ('Flash Attention', results['flash_attention']['pass']),
        ('Quantized Inference', results['quantized_inference']['pass']),
        ('Memory Reduction', results['memory_usage']['pass']),
        ('Multi-GPU Efficiency', results['multi_gpu']['pass']),
    ]
    
    for name, passed in tests:
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"  {name:<30} {status}")
    
    passed_count = sum(1 for _, p in tests if p)
    print()
    print(f"  {'OVERALL:':<30} {passed_count}/{len(tests)} tests passed")
    print()
    
    if passed_count == len(tests):
        print("🎉 " + "=" * 76 + " 🎉")
        print("   ALL PERFORMANCE CLAIMS VALIDATED!")
        print("   Nova is production-ready and delivers on all promises!")
        print("🎉 " + "=" * 76 + " 🎉")
    
    print()

if __name__ == '__main__':
    main()
