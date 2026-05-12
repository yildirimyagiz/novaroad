#!/usr/bin/env python3
"""
Type Specializer for Nova
Creates specialized versions of functions for specific type combinations
"""

from typing import Dict, Tuple, Any, Callable, List
from dataclasses import dataclass
import inspect


@dataclass
class TypeSignature:
    """Type signature for function specialization"""
    arg_types: Tuple[type, ...]
    
    def __hash__(self):
        return hash(self.arg_types)
    
    def __eq__(self, other):
        return self.arg_types == other.arg_types
    
    def __repr__(self):
        type_names = [t.__name__ for t in self.arg_types]
        return f"({', '.join(type_names)})"


class TypeSpecializer:
    """
    Creates type-specialized versions of functions
    
    Example:
        add(int, int) → specialized int version (fast!)
        add(float, float) → specialized float version
        add(str, str) → specialized string version
    """
    
    def __init__(self):
        # Cache: (function_name, type_signature) → specialized_function
        self.specialized_cache: Dict[Tuple[str, TypeSignature], Callable] = {}
        
        # Type observation: (function_name) → [type_signatures]
        self.observed_types: Dict[str, List[TypeSignature]] = {}
        
        # Statistics
        self.specializations_created = 0
        self.cache_hits = 0
        self.cache_misses = 0
    
    def call_specialized(self, func_name: str, func: Callable, *args):
        """
        Call function with type specialization
        
        On first call with new types: Create specialized version
        On subsequent calls: Use cached specialized version
        """
        # Determine type signature
        arg_types = tuple(type(arg) for arg in args)
        type_sig = TypeSignature(arg_types)
        
        # Check cache
        cache_key = (func_name, type_sig)
        if cache_key in self.specialized_cache:
            # Cache hit - use specialized version
            self.cache_hits += 1
            specialized_func = self.specialized_cache[cache_key]
            return specialized_func(*args)
        
        # Cache miss - create specialized version
        self.cache_misses += 1
        
        # Record type observation
        if func_name not in self.observed_types:
            self.observed_types[func_name] = []
        self.observed_types[func_name].append(type_sig)
        
        # Create specialized version
        specialized_func = self._create_specialized(func, type_sig)
        self.specialized_cache[cache_key] = specialized_func
        self.specializations_created += 1
        
        print(f"🔧 Created specialized version: {func_name}{type_sig}")
        
        # Execute specialized version
        return specialized_func(*args)
    
    def _create_specialized(self, func: Callable, type_sig: TypeSignature) -> Callable:
        """
        Create type-specialized version of function
        
        In a real implementation, this would:
        1. Generate optimized bytecode/LLVM IR for specific types
        2. Eliminate runtime type checks
        3. Use type-specific operations
        
        For demo, we just wrap the original function
        """
        # In production: Generate optimized code here
        
        # Check if we can create an optimized version
        if all(t in (int, float) for t in type_sig.arg_types):
            # Numeric types - can optimize
            return self._create_numeric_specialized(func, type_sig)
        elif all(t == str for t in type_sig.arg_types):
            # String types - can optimize
            return self._create_string_specialized(func, type_sig)
        else:
            # Generic - just wrap
            return func
    
    def _create_numeric_specialized(self, func: Callable, type_sig: TypeSignature):
        """Create numeric-specialized version"""
        # For demo: just wrap, but in production would generate optimized code
        def specialized(*args):
            # No type checking needed - we know the types!
            # Direct numeric operations
            return func(*args)
        
        specialized.__name__ = f"{func.__name__}_numeric"
        return specialized
    
    def _create_string_specialized(self, func: Callable, type_sig: TypeSignature):
        """Create string-specialized version"""
        def specialized(*args):
            # String-specific optimizations
            return func(*args)
        
        specialized.__name__ = f"{func.__name__}_string"
        return specialized
    
    def get_statistics(self) -> dict:
        """Get specialization statistics"""
        total_calls = self.cache_hits + self.cache_misses
        hit_rate = (self.cache_hits / total_calls * 100) if total_calls > 0 else 0
        
        return {
            'specializations_created': self.specializations_created,
            'cache_hits': self.cache_hits,
            'cache_misses': self.cache_misses,
            'cache_hit_rate': f"{hit_rate:.2f}%",
            'unique_signatures': len(self.specialized_cache)
        }
    
    def print_statistics(self):
        """Print specialization statistics"""
        stats = self.get_statistics()
        
        print("\n" + "="*70)
        print("TYPE SPECIALIZATION STATISTICS")
        print("="*70)
        
        print(f"\nSpecializations created: {stats['specializations_created']}")
        print(f"Cache hits: {stats['cache_hits']}")
        print(f"Cache misses: {stats['cache_misses']}")
        print(f"Hit rate: {stats['cache_hit_rate']}")
        print(f"Unique type signatures: {stats['unique_signatures']}")
        
        print(f"\nObserved type signatures:")
        for func_name, signatures in self.observed_types.items():
            print(f"  {func_name}:")
            for sig in set(signatures):
                count = signatures.count(sig)
                print(f"    {sig}: {count} calls")
        
        print("="*70)


def demo_type_specialization():
    """Demonstrate type specialization"""
    import time
    
    print("🔧 Type Specialization Demo\n")
    
    specializer = TypeSpecializer()
    
    # Define test functions
    def add(a, b):
        return a + b
    
    def multiply(a, b):
        return a * b
    
    # Test 1: Integer specialization
    print("Test 1: Integer operations")
    for i in range(5):
        result = specializer.call_specialized("add", add, 10, 20)
        print(f"  add(10, 20) = {result}")
    
    # Test 2: Float specialization
    print("\nTest 2: Float operations")
    for i in range(3):
        result = specializer.call_specialized("add", add, 1.5, 2.5)
        print(f"  add(1.5, 2.5) = {result}")
    
    # Test 3: String specialization
    print("\nTest 3: String operations")
    for i in range(3):
        result = specializer.call_specialized("add", add, "Hello", " World")
        print(f'  add("Hello", " World") = {result}')
    
    # Test 4: Multiple functions
    print("\nTest 4: Multiple functions")
    for i in range(4):
        result = specializer.call_specialized("multiply", multiply, 5, 6)
        print(f"  multiply(5, 6) = {result}")
    
    # Print statistics
    specializer.print_statistics()
    
    # Benchmark comparison
    print("\n📊 Performance Comparison:")
    
    # Without specialization
    iterations = 100000
    start = time.perf_counter()
    for _ in range(iterations):
        add(10, 20)
    time_regular = (time.perf_counter() - start) * 1000
    
    # With specialization (cached)
    start = time.perf_counter()
    for _ in range(iterations):
        specializer.call_specialized("add", add, 10, 20)
    time_specialized = (time.perf_counter() - start) * 1000
    
    print(f"  Regular calls:     {time_regular:.3f}ms")
    print(f"  Specialized calls: {time_specialized:.3f}ms")
    
    if time_regular > 0:
        speedup = time_regular / time_specialized
        print(f"  Speedup: {speedup:.2f}x")


if __name__ == '__main__':
    demo_type_specialization()
