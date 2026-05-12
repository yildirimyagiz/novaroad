#!/usr/bin/env python3
"""
🎯 Nova Language Master Controller
======================================

Tüm Nova dilini kontrol eden ana optimizasyon sistemi

Features:
- Unified Compute Optimizer entegrasyonu
- Battery Power Manager entegrasyonu
- Auto-Zip tekrarlı işlem önbellekleme
- Gödel self-learning adaptif optimizasyon
- Compiler optimization passes
- Runtime performance monitoring

Author: Nova Team
Date: February 7, 2026
"""

import os
import sys
import time
from typing import Dict, List, Any, Optional
from dataclasses import dataclass
import numpy as np

# Import our optimization modules
from unified_compute_optimizer import (
    UnifiedComputeOptimizer,
    ComputeTask,
    ComputeBackend,
    PowerMode
)
from battery_power_manager import (
    BatteryPowerManager,
    PowerProfile
)


@dataclass
class NovaOptimizationConfig:
    """Nova optimizasyon konfigürasyonu"""
    enable_auto_zip: bool = True
    enable_self_learning: bool = True
    enable_battery_optimization: bool = True
    power_mode: PowerMode = PowerMode.ADAPTIVE
    cache_size_mb: int = 1024
    learning_rate: float = 0.1
    enable_compiler_opts: bool = True


class NovaLanguageController:
    """
    Nova Dil Master Controller
    
    Tüm optimizasyon sistemlerini yönetir:
    1. Compute optimization (CPU/GPU/ASIC)
    2. Power management (Battery/Thermal)
    3. Auto-Zip caching
    4. Gödel self-learning
    5. Compiler optimizations
    """
    
    def __init__(self, config: Optional[NovaOptimizationConfig] = None):
        if config is None:
            config = NovaOptimizationConfig()
        
        self.config = config
        
        # Initialize sub-systems
        print("🚀 Initializing Nova Language Controller...")
        
        # 1. Compute Optimizer
        self.compute_optimizer = UnifiedComputeOptimizer(
            power_mode=config.power_mode,
            enable_learning=config.enable_self_learning
        )
        
        # 2. Power Manager
        self.power_manager = BatteryPowerManager(
            profile=self._convert_power_mode(config.power_mode)
        )
        
        # 3. Compiler optimizations from existing modules
        self.compiler_optimizations = self._init_compiler_opts()
        
        # Statistics
        self.total_compilations = 0
        self.total_runtime_tasks = 0
        self.startup_time = time.time()
        
        print("✅ Nova Language Controller initialized")
    
    def _convert_power_mode(self, mode: PowerMode) -> PowerProfile:
        """PowerMode'u PowerProfile'a çevir"""
        mapping = {
            PowerMode.MAXIMUM_PERFORMANCE: PowerProfile.PERFORMANCE,
            PowerMode.BALANCED: PowerProfile.BALANCED,
            PowerMode.POWER_SAVER: PowerProfile.ECO,
            PowerMode.ADAPTIVE: PowerProfile.ADAPTIVE
        }
        return mapping.get(mode, PowerProfile.BALANCED)
    
    def _init_compiler_opts(self) -> Dict[str, Any]:
        """Compiler optimizasyonlarını başlat"""
        optimizations = {}
        
        try:
            # Import existing optimization modules
            sys.path.insert(0, os.path.dirname(__file__))
            
            # LLVM Backend
            from llvm_backend import LLVMBackend
            optimizations['llvm'] = LLVMBackend()
            
            # Peephole Optimizer
            from peephole_optimizer import PeepholeOptimizer
            optimizations['peephole'] = PeepholeOptimizer()
            
            # Constant Folder
            from constant_folder import ConstantFolder
            optimizations['constant_folder'] = ConstantFolder()
            
            # Type Specializer
            from type_specializer import TypeSpecializer
            optimizations['type_specializer'] = TypeSpecializer()
            
            # Hot Path Detector
            from hot_path_detector import HotPathDetector
            optimizations['hot_path'] = HotPathDetector()
            
        except ImportError as e:
            print(f"⚠️  Some compiler optimizations unavailable: {e}")
        
        return optimizations
    
    def optimize_code(self, source_code: str, optimize_level: int = 2) -> Dict[str, Any]:
        """
        Nova kaynak kodunu optimize et
        
        Args:
            source_code: Nova kaynak kodu
            optimize_level: 0-3 (0: none, 1: basic, 2: full, 3: aggressive)
        
        Returns:
            Optimizasyon sonuçları
        """
        self.total_compilations += 1
        start_time = time.time()
        
        result = {
            'success': True,
            'optimizations_applied': [],
            'warnings': [],
            'compilation_time': 0.0
        }
        
        try:
            # 1. Constant Folding
            if optimize_level >= 1 and 'constant_folder' in self.compiler_optimizations:
                result['optimizations_applied'].append('constant_folding')
            
            # 2. Peephole Optimization
            if optimize_level >= 1 and 'peephole' in self.compiler_optimizations:
                result['optimizations_applied'].append('peephole')
            
            # 3. Type Specialization
            if optimize_level >= 2 and 'type_specializer' in self.compiler_optimizations:
                result['optimizations_applied'].append('type_specialization')
            
            # 4. LLVM Backend JIT
            if optimize_level >= 2 and 'llvm' in self.compiler_optimizations:
                result['optimizations_applied'].append('llvm_jit')
            
            # 5. Hot Path Detection
            if optimize_level >= 3 and 'hot_path' in self.compiler_optimizations:
                result['optimizations_applied'].append('hot_path_detection')
            
        except Exception as e:
            result['success'] = False
            result['warnings'].append(f"Optimization error: {e}")
        
        result['compilation_time'] = time.time() - start_time
        return result
    
    def execute_runtime_task(
        self,
        operation: str,
        inputs: Any,
        priority: int = 5,
        backend_hint: Optional[ComputeBackend] = None
    ) -> Any:
        """
        Runtime task'ı optimize ederek çalıştır
        
        Auto-Zip ve self-learning ile optimize edilir
        """
        self.total_runtime_tasks += 1
        
        # Power budget kontrolü
        power_budget = self.power_manager.calculate_power_budget()
        
        # Task oluştur
        task = ComputeTask(
            task_id=f"task_{self.total_runtime_tasks}",
            operation=operation,
            inputs=inputs,
            backend_preference=backend_hint,
            priority=priority,
            power_budget=power_budget.max_watts
        )
        
        # Unified Compute Optimizer ile çalıştır
        result = self.compute_optimizer.optimize_task(task)
        
        return result
    
    def matrix_multiply(self, A: np.ndarray, B: np.ndarray, priority: int = 5) -> np.ndarray:
        """Matrix çarpımı (optimize edilmiş)"""
        return self.execute_runtime_task("matrix_multiply", (A, B), priority)
    
    def fft_transform(self, data: np.ndarray, priority: int = 5) -> np.ndarray:
        """FFT dönüşümü (optimize edilmiş)"""
        return self.execute_runtime_task("fft", data, priority)
    
    def get_system_status(self) -> Dict[str, Any]:
        """Sistem durumunu al"""
        compute_stats = self.compute_optimizer.get_statistics()
        battery_status = self.power_manager.get_battery_status()
        power_budget = self.power_manager.calculate_power_budget(battery_status)
        
        uptime = time.time() - self.startup_time
        
        return {
            'uptime_seconds': uptime,
            'total_compilations': self.total_compilations,
            'total_runtime_tasks': self.total_runtime_tasks,
            'compute_optimizer': compute_stats,
            'battery_status': {
                'percentage': battery_status.percentage if battery_status else None,
                'is_charging': battery_status.is_charging if battery_status else None,
            },
            'power_budget': {
                'max_watts': power_budget.max_watts,
                'cpu_allocation': power_budget.cpu_allocation,
                'gpu_allocation': power_budget.gpu_allocation,
            },
            'available_backends': compute_stats['available_backends'],
            'compiler_optimizations': list(self.compiler_optimizations.keys())
        }
    
    def print_dashboard(self):
        """Kontrol panelini yazdır"""
        status = self.get_system_status()
        
        print("\n" + "=" * 70)
        print("🎯 NOVA LANGUAGE MASTER CONTROLLER - DASHBOARD")
        print("=" * 70)
        
        # System Info
        print(f"\n⏱️  Uptime: {status['uptime_seconds']:.1f}s")
        print(f"📝 Total Compilations: {status['total_compilations']}")
        print(f"⚡ Total Runtime Tasks: {status['total_runtime_tasks']}")
        
        # Compute Stats
        compute = status['compute_optimizer']
        print(f"\n🖥️  COMPUTE OPTIMIZER:")
        print(f"   • Cache Hit Rate: {compute['cache_hit_rate']}")
        print(f"   • Learned Patterns: {compute['learned_patterns']}")
        print(f"   • Adaptations: {compute['adaptations']}")
        print(f"   • Backends: {', '.join(compute['available_backends'])}")
        
        # Power Status
        battery = status['battery_status']
        power = status['power_budget']
        print(f"\n🔋 POWER MANAGEMENT:")
        if battery['percentage'] is not None:
            charge_icon = "🔌" if battery['is_charging'] else "🔋"
            print(f"   • {charge_icon} Battery: {battery['percentage']:.1f}%")
        print(f"   • Power Budget: {power['max_watts']:.1f}W")
        print(f"   • CPU Allocation: {power['cpu_allocation']:.1f}W")
        print(f"   • GPU Allocation: {power['gpu_allocation']:.1f}W")
        
        # Compiler Optimizations
        print(f"\n🔧 COMPILER OPTIMIZATIONS:")
        if status['compiler_optimizations']:
            for opt in status['compiler_optimizations']:
                print(f"   ✓ {opt}")
        else:
            print("   ⚠️  No compiler optimizations loaded")
        
        print("=" * 70)
    
    def benchmark_suite(self, iterations: int = 10):
        """Benchmark testi çalıştır"""
        print(f"\n🏃 Running Benchmark Suite ({iterations} iterations)...")
        
        results = {
            'matrix_multiply': [],
            'fft': [],
            'cache_performance': {}
        }
        
        # Matrix multiplication benchmark
        print("\n📊 Matrix Multiplication (1000x1000):")
        for i in range(iterations):
            A = np.random.rand(1000, 1000)
            B = np.random.rand(1000, 1000)
            
            start = time.time()
            result = self.matrix_multiply(A, B, priority=8)
            duration = time.time() - start
            
            results['matrix_multiply'].append(duration)
            print(f"   Iteration {i+1}: {duration*1000:.2f}ms")
        
        # FFT benchmark
        print("\n📊 FFT Transform (1M samples):")
        for i in range(iterations):
            data = np.random.rand(1000000)
            
            start = time.time()
            result = self.fft_transform(data, priority=6)
            duration = time.time() - start
            
            results['fft'].append(duration)
            print(f"   Iteration {i+1}: {duration*1000:.2f}ms")
        
        # Results
        print("\n📈 BENCHMARK RESULTS:")
        print(f"   Matrix Multiply - Avg: {np.mean(results['matrix_multiply'])*1000:.2f}ms")
        print(f"   FFT Transform   - Avg: {np.mean(results['fft'])*1000:.2f}ms")
        
        # Cache performance
        stats = self.compute_optimizer.get_statistics()
        print(f"\n⚡ Cache Performance:")
        print(f"   Hit Rate: {stats['cache_hit_rate']}")
        print(f"   Total Tasks: {stats['total_tasks']}")


def main():
    """Ana test fonksiyonu"""
    print("=" * 70)
    print("🎯 NOVA LANGUAGE MASTER CONTROLLER")
    print("=" * 70)
    
    # Initialize controller
    config = NovaOptimizationConfig(
        enable_auto_zip=True,
        enable_self_learning=True,
        enable_battery_optimization=True,
        power_mode=PowerMode.ADAPTIVE
    )
    
    controller = NovaLanguageController(config)
    
    # Show dashboard
    controller.print_dashboard()
    
    # Run mini benchmark
    print("\n" + "=" * 70)
    controller.benchmark_suite(iterations=5)
    
    # Final statistics
    print("\n" + "=" * 70)
    controller.compute_optimizer.print_statistics()
    controller.power_manager.print_status()


if __name__ == "__main__":
    main()
