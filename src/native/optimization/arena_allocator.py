#!/usr/bin/env python3
"""
Arena Allocator for Nova Real-Time Systems
Provides O(1) deterministic memory allocation
"""

from typing import Optional, List
import sys


class Arena:
    """
    Arena (region-based) memory allocator
    
    Features:
    - O(1) allocation (deterministic!)
    - Bulk deallocation (reset entire arena)
    - No fragmentation
    - Perfect for real-time systems
    
    Usage:
        arena = Arena(1024 * 1024)  # 1MB
        obj = arena.alloc(MyClass())
        # ... use object ...
        arena.reset()  # Free all at once!
    """
    
    def __init__(self, size_bytes: int):
        self.size = size_bytes
        self.buffer = bytearray(size_bytes)
        self.offset = 0
        
        # Statistics
        self.allocations = 0
        self.resets = 0
        self.peak_usage = 0
        self.total_allocated = 0
    
    def alloc(self, size: int, alignment: int = 8) -> Optional[int]:
        """
        Allocate memory from arena
        
        Args:
            size: Number of bytes to allocate
            alignment: Alignment requirement (default 8 bytes)
        
        Returns:
            Offset into buffer, or None if out of memory
        """
        # Align offset
        aligned_offset = (self.offset + alignment - 1) & ~(alignment - 1)
        
        # Check if we have space
        if aligned_offset + size > self.size:
            return None  # Out of memory
        
        # Allocate
        result = aligned_offset
        self.offset = aligned_offset + size
        
        # Update statistics
        self.allocations += 1
        self.total_allocated += size
        if self.offset > self.peak_usage:
            self.peak_usage = self.offset
        
        return result
    
    def reset(self):
        """
        Reset arena - deallocate all at once!
        
        O(1) operation - just reset offset
        Perfect for per-frame allocation in games/real-time systems
        """
        self.offset = 0
        self.resets += 1
    
    def get_usage(self) -> float:
        """Get current memory usage as percentage"""
        return (self.offset / self.size * 100) if self.size > 0 else 0
    
    def get_statistics(self) -> dict:
        """Get arena statistics"""
        return {
            'size_bytes': self.size,
            'current_offset': self.offset,
            'usage_percent': self.get_usage(),
            'peak_usage_bytes': self.peak_usage,
            'peak_usage_percent': (self.peak_usage / self.size * 100),
            'allocations': self.allocations,
            'resets': self.resets,
            'total_allocated_bytes': self.total_allocated,
        }
    
    def print_statistics(self):
        """Print arena statistics"""
        stats = self.get_statistics()
        
        print("\n" + "="*70)
        print("ARENA ALLOCATOR STATISTICS")
        print("="*70)
        
        print(f"\nCapacity: {stats['size_bytes']:,} bytes")
        print(f"Current usage: {stats['current_offset']:,} bytes ({stats['usage_percent']:.1f}%)")
        print(f"Peak usage: {stats['peak_usage_bytes']:,} bytes ({stats['peak_usage_percent']:.1f}%)")
        print(f"\nAllocations: {stats['allocations']}")
        print(f"Resets: {stats['resets']}")
        print(f"Total allocated: {stats['total_allocated_bytes']:,} bytes")
        
        print("="*70)


class ObjectArena:
    """
    Object-level arena allocator
    
    Manages Python objects in an arena for easy cleanup
    """
    
    def __init__(self, capacity: int = 10000):
        self.capacity = capacity
        self.objects: List = []
    
    def alloc(self, obj):
        """Allocate object in arena"""
        if len(self.objects) >= self.capacity:
            raise MemoryError("Arena full")
        
        self.objects.append(obj)
        return obj
    
    def reset(self):
        """Clear all objects"""
        self.objects.clear()
    
    def __len__(self):
        return len(self.objects)


def demo_arena_allocator():
    """Demonstrate arena allocator for real-time systems"""
    import time
    
    print("🏟️  Arena Allocator Demo - Real-Time Simulation\n")
    
    # Create arena
    arena = Arena(1024 * 1024)  # 1MB
    
    print("Simulating real-time control loop...")
    print("(Each iteration = one control cycle)\n")
    
    for cycle in range(5):
        print(f"Cycle {cycle + 1}:")
        
        # Allocate memory for this cycle
        for i in range(100):
            size = 1024  # 1KB per object
            offset = arena.alloc(size)
            
            if offset is None:
                print("  ❌ Out of memory!")
                break
        
        usage = arena.get_usage()
        print(f"  Allocated 100 objects")
        print(f"  Memory usage: {usage:.1f}%")
        
        # Simulate processing
        time.sleep(0.1)
        
        # End of cycle - reset arena (O(1)!)
        arena.reset()
        print(f"  ✅ Arena reset (O(1) deallocation!)")
        print()
    
    # Print statistics
    arena.print_statistics()
    
    print("\n✅ Benefits for Real-Time Systems:")
    print("  • O(1) allocation (deterministic!)")
    print("  • O(1) bulk deallocation")
    print("  • No GC pauses")
    print("  • No fragmentation")
    print("  • Predictable worst-case performance")


if __name__ == '__main__':
    demo_arena_allocator()
