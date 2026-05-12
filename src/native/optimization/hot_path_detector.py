#!/usr/bin/env python3
"""
Hot Path Detector for Nova JIT
Identifies frequently executed code paths for optimization
"""

import time
from typing import Dict, List, Set, Optional
from dataclasses import dataclass
from collections import defaultdict


@dataclass
class HotSpot:
    """Represents a hot code path"""
    identifier: str
    execution_count: int
    total_time_ms: float
    avg_time_ms: float
    is_hot: bool = False
    jit_compiled: bool = False


class HotPathDetector:
    """
    Detects hot paths for JIT compilation
    
    Strategies:
    1. Count-based: Execute N times → compile
    2. Time-based: Takes X% of total time → compile
    3. Trace-based: Record execution traces
    """
    
    def __init__(self, 
                 count_threshold: int = 100,
                 time_threshold_percent: float = 5.0):
        
        # Configuration
        self.count_threshold = count_threshold
        self.time_threshold_percent = time_threshold_percent
        
        # Tracking data
        self.execution_counts: Dict[str, int] = defaultdict(int)
        self.execution_times: Dict[str, List[float]] = defaultdict(list)
        self.hot_paths: Set[str] = set()
        self.compiled_paths: Set[str] = set()
        
        # Trace recording
        self.traces: Dict[str, List[str]] = defaultdict(list)
        self.current_trace: Optional[List[str]] = None
        
        # Statistics
        self.total_executions = 0
        self.total_time_ms = 0.0
    
    def record_execution(self, identifier: str, execution_time_ms: float):
        """Record function/loop execution"""
        self.execution_counts[identifier] += 1
        self.execution_times[identifier].append(execution_time_ms)
        self.total_executions += 1
        self.total_time_ms += execution_time_ms
        
        # Check if this path just became hot
        if self._is_hot(identifier) and identifier not in self.hot_paths:
            self.hot_paths.add(identifier)
            return True  # New hot path detected!
        
        return False
    
    def _is_hot(self, identifier: str) -> bool:
        """Check if path is hot"""
        count = self.execution_counts[identifier]
        
        # Count-based threshold
        if count >= self.count_threshold:
            return True
        
        # Time-based threshold
        if count >= 10:  # Need minimum samples
            total_time = sum(self.execution_times[identifier])
            time_percent = (total_time / self.total_time_ms * 100) if self.total_time_ms > 0 else 0
            
            if time_percent >= self.time_threshold_percent:
                return True
        
        return False
    
    def start_trace(self, identifier: str):
        """Start recording execution trace"""
        self.current_trace = []
        self.current_trace.append(identifier)
    
    def record_trace_step(self, step: str):
        """Record step in current trace"""
        if self.current_trace is not None:
            self.current_trace.append(step)
    
    def end_trace(self, identifier: str):
        """End trace recording"""
        if self.current_trace is not None:
            self.traces[identifier].append(self.current_trace)
            self.current_trace = None
    
    def get_hot_spots(self) -> List[HotSpot]:
        """Get list of hot spots sorted by importance"""
        hot_spots = []
        
        for identifier, count in self.execution_counts.items():
            if count < 5:  # Need minimum samples
                continue
            
            times = self.execution_times[identifier]
            total_time = sum(times)
            avg_time = total_time / len(times)
            is_hot = identifier in self.hot_paths
            is_compiled = identifier in self.compiled_paths
            
            hot_spot = HotSpot(
                identifier=identifier,
                execution_count=count,
                total_time_ms=total_time,
                avg_time_ms=avg_time,
                is_hot=is_hot,
                jit_compiled=is_compiled
            )
            
            hot_spots.append(hot_spot)
        
        # Sort by total time (most time-consuming first)
        hot_spots.sort(key=lambda x: x.total_time_ms, reverse=True)
        
        return hot_spots
    
    def mark_compiled(self, identifier: str):
        """Mark path as JIT compiled"""
        self.compiled_paths.add(identifier)
    
    def should_compile(self, identifier: str) -> bool:
        """Check if path should be JIT compiled"""
        return (identifier in self.hot_paths and 
                identifier not in self.compiled_paths)
    
    def get_trace(self, identifier: str) -> Optional[List[List[str]]]:
        """Get execution traces for identifier"""
        return self.traces.get(identifier)
    
    def print_hot_spots(self):
        """Print hot spots report"""
        hot_spots = self.get_hot_spots()
        
        print("\n" + "="*80)
        print("HOT PATH DETECTION REPORT")
        print("="*80)
        
        print(f"\nTotal executions: {self.total_executions}")
        print(f"Total time: {self.total_time_ms:.2f}ms")
        print(f"Hot paths detected: {len(self.hot_paths)}")
        print(f"Compiled paths: {len(self.compiled_paths)}")
        
        print(f"\n{'Identifier':<40} {'Count':<10} {'Total(ms)':<12} {'Avg(ms)':<10} {'Status'}")
        print("-"*80)
        
        for spot in hot_spots[:20]:  # Top 20
            status = ""
            if spot.jit_compiled:
                status = "JIT ✅"
            elif spot.is_hot:
                status = "HOT 🔥"
            else:
                status = "WARM"
            
            print(f"{spot.identifier:<40} {spot.execution_count:<10} "
                  f"{spot.total_time_ms:>10.2f}  {spot.avg_time_ms:>8.4f}  {status}")
        
        print("="*80)


class ProfilingContext:
    """Context manager for profiling code sections"""
    
    def __init__(self, detector: HotPathDetector, identifier: str):
        self.detector = detector
        self.identifier = identifier
        self.start_time = 0.0
    
    def __enter__(self):
        self.start_time = time.perf_counter()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        end_time = time.perf_counter()
        execution_time_ms = (end_time - self.start_time) * 1000
        
        new_hot = self.detector.record_execution(self.identifier, execution_time_ms)
        
        if new_hot:
            print(f"🔥 New hot path detected: {self.identifier}")


def demo_hot_path_detection():
    """Demonstrate hot path detection"""
    import random
    
    print("🔥 Hot Path Detector Demo\n")
    
    detector = HotPathDetector(count_threshold=50, time_threshold_percent=10.0)
    
    # Simulate function executions
    
    # Hot function 1: Called many times
    print("Simulating hot_function_1 (100 calls)...")
    for i in range(100):
        with ProfilingContext(detector, "hot_function_1"):
            time.sleep(0.001)  # 1ms each
    
    # Hot function 2: Called fewer times but slower
    print("Simulating hot_function_2 (30 calls, slow)...")
    for i in range(30):
        with ProfilingContext(detector, "hot_function_2"):
            time.sleep(0.005)  # 5ms each
    
    # Cold function: Rarely called
    print("Simulating cold_function (10 calls)...")
    for i in range(10):
        with ProfilingContext(detector, "cold_function"):
            time.sleep(0.0001)  # 0.1ms each
    
    # Medium function: Called moderately
    print("Simulating medium_function (40 calls)...")
    for i in range(40):
        with ProfilingContext(detector, "medium_function"):
            time.sleep(0.002)  # 2ms each
    
    # Print report
    detector.print_hot_spots()
    
    # Show which should be compiled
    print("\n🎯 Compilation Recommendations:")
    for identifier in detector.hot_paths:
        if detector.should_compile(identifier):
            print(f"  ✅ Compile: {identifier}")


if __name__ == '__main__':
    demo_hot_path_detection()
