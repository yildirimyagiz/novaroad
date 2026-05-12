#!/usr/bin/env python3
"""
📊 Nova Comprehensive Benchmark Suite
========================================

Tüm optimizasyon sistemlerini test eden kapsamlı benchmark

Author: Nova Team
Date: February 7, 2026
"""

import time
import numpy as np
from typing import Dict, List
from nova_language_controller import NovaLanguageController, NovaOptimizationConfig
from unified_compute_optimizer import PowerMode, ComputeBackend


class ComprehensiveBenchmark:
    """Kapsamlı benchmark suite"""
    
    def __init__(self):
        self.results = {}
        self.controller = None
    
    def run_all_tests(self):
        """Tüm testleri çalıştır"""
        print("=" * 80)
        print("📊 NOVA COMPREHENSIVE BENCHMARK SUITE")
        print("=" * 80)
        
        # Test 1: Auto-Zip Cache Performance
        print("\n🧪 TEST 1: Auto-Zip Cache Performance")
        self.test_auto_zip_cache()
        
        # Test 2: Backend Selection
        print("\n🧪 TEST 2: Backend Selection & Performance")
        self.test_backend_selection()
        
        # Test 3: Self-Learning Adaptation
        print("\n🧪 TEST 3: Gödel Self-Learning Adaptation")
        self.test_self_learning()
        
        # Test 4: Power Mode Comparison
        print("\n🧪 TEST 4: Power Mode Performance Comparison")
        self.test_power_modes()
        
        # Test 5: Battery Optimization
        print("\n🧪 TEST 5: Battery Optimization Impact")
        self.test_battery_optimization()
        
        # Final Report
        self.print_final_report()
    
    def test_auto_zip_cache(self):
        """Auto-Zip cache performansını test et"""
        config = NovaOptimizationConfig(enable_auto_zip=True)
        controller = NovaLanguageController(config)
        
        # Test data
        A = np.random.rand(500, 500)
        B = np.random.rand(500, 500)
        
        # First run (cache miss)
        start = time.time()
        for i in range(10):
            result = controller.matrix_multiply(A, B)
        first_run_time = time.time() - start
        
        # Second run (cache hit)
        start = time.time()
        for i in range(10):
            result = controller.matrix_multiply(A, B)
        second_run_time = time.time() - start
        
        speedup = first_run_time / second_run_time if second_run_time > 0 else 1.0
        
        print(f"   First Run (no cache):  {first_run_time*1000:.2f}ms")
        print(f"   Second Run (cached):   {second_run_time*1000:.2f}ms")
        print(f"   🚀 Speedup: {speedup:.2f}x")
        
        stats = controller.compute_optimizer.get_statistics()
        print(f"   Cache Hit Rate: {stats['cache_hit_rate']}")
        
        self.results['auto_zip'] = {
            'speedup': speedup,
            'cache_hit_rate': stats['cache_hit_rate']
        }
    
    def test_backend_selection(self):
        """Backend seçimini test et"""
        config = NovaOptimizationConfig()
        controller = NovaLanguageController(config)
        
        backends = controller.compute_optimizer.available_backends
        print(f"   Available Backends: {[b.value for b in backends]}")
        
        # Test matrix multiplication on each backend
        A = np.random.rand(1000, 1000)
        B = np.random.rand(1000, 1000)
        
        backend_times = {}
        for backend in backends:
            start = time.time()
            result = controller.execute_runtime_task(
                "matrix_multiply",
                (A, B),
                priority=10,
                backend_hint=backend
            )
            duration = time.time() - start
            backend_times[backend.value] = duration
            print(f"   {backend.value.upper()}: {duration*1000:.2f}ms")
        
        self.results['backends'] = backend_times
    
    def test_self_learning(self):
        """Self-learning adaptasyonunu test et"""
        config = NovaOptimizationConfig(enable_self_learning=True)
        controller = NovaLanguageController(config)
        
        # Aynı operasyonu tekrarla
        A = np.random.rand(300, 300)
        B = np.random.rand(300, 300)
        
        initial_stats = controller.compute_optimizer.get_statistics()
        
        # 50 iterasyon
        for i in range(50):
            result = controller.matrix_multiply(A, B, priority=7)
        
        final_stats = controller.compute_optimizer.get_statistics()
        
        print(f"   Initial Adaptations: {initial_stats['adaptations']}")
        print(f"   Final Adaptations: {final_stats['adaptations']}")
        print(f"   Learned Patterns: {final_stats['learned_patterns']}")
        
        # Öğrenilen pattern'leri göster
        if controller.compute_optimizer.learned_patterns:
            for op, data in controller.compute_optimizer.learned_patterns.items():
                print(f"   📚 Pattern '{op}': best backend = {data.get('best_backend')}")
        
        self.results['self_learning'] = {
            'adaptations': final_stats['adaptations'],
            'learned_patterns': final_stats['learned_patterns']
        }
    
    def test_power_modes(self):
        """Farklı güç modlarını karşılaştır"""
        modes = [
            PowerMode.POWER_SAVER,
            PowerMode.BALANCED,
            PowerMode.ADAPTIVE,
            PowerMode.MAXIMUM_PERFORMANCE
        ]
        
        mode_results = {}
        
        for mode in modes:
            config = NovaOptimizationConfig(power_mode=mode)
            controller = NovaLanguageController(config)
            
            # Test
            A = np.random.rand(500, 500)
            B = np.random.rand(500, 500)
            
            start = time.time()
            for i in range(5):
                result = controller.matrix_multiply(A, B)
            duration = time.time() - start
            
            mode_results[mode.value] = duration
            print(f"   {mode.value.upper()}: {duration*1000:.2f}ms")
        
        self.results['power_modes'] = mode_results
    
    def test_battery_optimization(self):
        """Batarya optimizasyonunu test et"""
        config = NovaOptimizationConfig(enable_battery_optimization=True)
        controller = NovaLanguageController(config)
        
        battery_status = controller.power_manager.get_battery_status()
        power_budget = controller.power_manager.calculate_power_budget()
        
        if battery_status:
            print(f"   Battery Level: {battery_status.percentage:.1f}%")
            print(f"   Charging: {battery_status.is_charging}")
        else:
            print("   Battery info unavailable (desktop system)")
        
        print(f"   Power Budget: {power_budget.max_watts:.1f}W")
        print(f"   CPU Allocation: {power_budget.cpu_allocation:.1f}W")
        print(f"   GPU Allocation: {power_budget.gpu_allocation:.1f}W")
        
        self.results['battery'] = {
            'power_budget': power_budget.max_watts,
            'cpu_allocation': power_budget.cpu_allocation,
            'gpu_allocation': power_budget.gpu_allocation
        }
    
    def print_final_report(self):
        """Final raporu yazdır"""
        print("\n" + "=" * 80)
        print("📋 FINAL BENCHMARK REPORT")
        print("=" * 80)
        
        if 'auto_zip' in self.results:
            print(f"\n⚡ Auto-Zip Cache:")
            print(f"   Speedup: {self.results['auto_zip']['speedup']:.2f}x")
            print(f"   Hit Rate: {self.results['auto_zip']['cache_hit_rate']}")
        
        if 'backends' in self.results:
            print(f"\n🖥️  Backend Performance:")
            for backend, time in self.results['backends'].items():
                print(f"   {backend}: {time*1000:.2f}ms")
        
        if 'self_learning' in self.results:
            print(f"\n🧠 Self-Learning:")
            print(f"   Adaptations: {self.results['self_learning']['adaptations']}")
            print(f"   Learned Patterns: {self.results['self_learning']['learned_patterns']}")
        
        if 'power_modes' in self.results:
            print(f"\n🔋 Power Modes:")
            for mode, time in self.results['power_modes'].items():
                print(f"   {mode}: {time*1000:.2f}ms")
        
        print("\n" + "=" * 80)
        print("✅ ALL BENCHMARKS COMPLETED")
        print("=" * 80)


def main():
    """Ana test fonksiyonu"""
    benchmark = ComprehensiveBenchmark()
    benchmark.run_all_tests()


if __name__ == "__main__":
    main()
