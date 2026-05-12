#!/usr/bin/env python3
"""
Generational Garbage Collector for Nova
Reduces GC pauses with generational collection
"""

import time
from typing import List, Set, Dict, Any
from dataclasses import dataclass, field
from enum import Enum


class Generation(Enum):
    """Object generations"""
    YOUNG = 0    # Recently allocated objects
    OLD = 1      # Long-lived objects


@dataclass
class ManagedObject:
    """Object tracked by GC"""
    value: Any
    generation: Generation = Generation.YOUNG
    age: int = 0
    marked: bool = False
    references: List['ManagedObject'] = field(default_factory=list)
    
    def __hash__(self):
        return id(self)


class GenerationalGC:
    """
    Generational garbage collector
    
    Key ideas:
    - Most objects die young
    - Collect young generation frequently (fast)
    - Collect old generation rarely (slow but rare)
    - Reduces average pause time significantly
    """
    
    def __init__(self, 
                 young_threshold: int = 1000,
                 old_threshold: int = 10000,
                 promotion_age: int = 3):
        
        # Object storage
        self.young_gen: Set[ManagedObject] = set()
        self.old_gen: Set[ManagedObject] = set()
        
        # GC thresholds
        self.young_threshold = young_threshold
        self.old_threshold = old_threshold
        self.promotion_age = promotion_age
        
        # Roots (always reachable)
        self.roots: Set[ManagedObject] = set()
        
        # Statistics
        self.young_collections = 0
        self.old_collections = 0
        self.total_young_time_ms = 0.0
        self.total_old_time_ms = 0.0
        self.objects_promoted = 0
        self.objects_collected = 0
    
    def allocate(self, value: Any) -> ManagedObject:
        """Allocate new object in young generation"""
        obj = ManagedObject(value=value, generation=Generation.YOUNG)
        self.young_gen.add(obj)
        
        # Check if young generation is full
        if len(self.young_gen) >= self.young_threshold:
            self.collect_young()
        
        return obj
    
    def add_root(self, obj: ManagedObject):
        """Add object to root set (always reachable)"""
        self.roots.add(obj)
    
    def collect_young(self):
        """
        Collect young generation (minor GC)
        
        Fast collection:
        - Only scans young generation
        - Promotes survivors to old generation
        - Low pause time
        """
        start = time.perf_counter()
        
        # Mark phase - mark reachable objects
        for root in self.roots:
            if root.generation == Generation.YOUNG:
                self._mark_young(root)
        
        # Also mark young objects referenced from old generation
        for old_obj in self.old_gen:
            for ref in old_obj.references:
                if ref.generation == Generation.YOUNG:
                    self._mark_young(ref)
        
        # Sweep phase - collect unmarked objects
        to_remove = set()
        to_promote = set()
        
        for obj in self.young_gen:
            if obj.marked:
                # Survived - age it
                obj.age += 1
                obj.marked = False  # Reset for next collection
                
                # Promote to old generation if old enough
                if obj.age >= self.promotion_age:
                    to_promote.add(obj)
            else:
                # Not marked - garbage
                to_remove.add(obj)
        
        # Remove garbage
        self.young_gen -= to_remove
        self.objects_collected += len(to_remove)
        
        # Promote survivors
        for obj in to_promote:
            obj.generation = Generation.OLD
            self.young_gen.remove(obj)
            self.old_gen.add(obj)
            self.objects_promoted += 1
        
        # Update statistics
        end = time.perf_counter()
        pause_time = (end - start) * 1000
        self.young_collections += 1
        self.total_young_time_ms += pause_time
    
    def collect_old(self):
        """
        Collect old generation (major GC)
        
        Full collection:
        - Scans both generations
        - Slower but removes all garbage
        - Rare (only when old gen is full)
        """
        start = time.perf_counter()
        
        # Mark phase - mark all reachable
        for root in self.roots:
            self._mark(root)
        
        # Sweep phase - collect unmarked in both generations
        young_garbage = set()
        old_garbage = set()
        
        for obj in self.young_gen:
            if obj.marked:
                obj.marked = False
            else:
                young_garbage.add(obj)
        
        for obj in self.old_gen:
            if obj.marked:
                obj.marked = False
            else:
                old_garbage.add(obj)
        
        # Remove garbage
        self.young_gen -= young_garbage
        self.old_gen -= old_garbage
        self.objects_collected += len(young_garbage) + len(old_garbage)
        
        # Update statistics
        end = time.perf_counter()
        pause_time = (end - start) * 1000
        self.old_collections += 1
        self.total_old_time_ms += pause_time
    
    def _mark_young(self, obj: ManagedObject):
        """Mark object and its references (young gen only)"""
        if obj.marked:
            return
        
        obj.marked = True
        
        for ref in obj.references:
            if ref.generation == Generation.YOUNG:
                self._mark_young(ref)
    
    def _mark(self, obj: ManagedObject):
        """Mark object and all its references (full)"""
        if obj.marked:
            return
        
        obj.marked = True
        
        for ref in obj.references:
            self._mark(ref)
    
    def should_collect_old(self) -> bool:
        """Check if old generation should be collected"""
        return len(self.old_gen) >= self.old_threshold
    
    def get_statistics(self) -> Dict[str, Any]:
        """Get GC statistics"""
        total_collections = self.young_collections + self.old_collections
        avg_young_pause = (self.total_young_time_ms / self.young_collections 
                          if self.young_collections > 0 else 0)
        avg_old_pause = (self.total_old_time_ms / self.old_collections 
                        if self.old_collections > 0 else 0)
        
        return {
            'young_collections': self.young_collections,
            'old_collections': self.old_collections,
            'total_collections': total_collections,
            'avg_young_pause_ms': avg_young_pause,
            'avg_old_pause_ms': avg_old_pause,
            'total_pause_time_ms': self.total_young_time_ms + self.total_old_time_ms,
            'objects_promoted': self.objects_promoted,
            'objects_collected': self.objects_collected,
            'current_young_objects': len(self.young_gen),
            'current_old_objects': len(self.old_gen),
        }
    
    def print_statistics(self):
        """Print GC statistics"""
        stats = self.get_statistics()
        
        print("\n" + "="*70)
        print("GENERATIONAL GC STATISTICS")
        print("="*70)
        
        print(f"\nCollections:")
        print(f"  Young (minor GC): {stats['young_collections']}")
        print(f"  Old (major GC):   {stats['old_collections']}")
        print(f"  Total:            {stats['total_collections']}")
        
        print(f"\nPause Times:")
        print(f"  Avg young pause: {stats['avg_young_pause_ms']:.3f}ms")
        print(f"  Avg old pause:   {stats['avg_old_pause_ms']:.3f}ms")
        print(f"  Total time:      {stats['total_pause_time_ms']:.3f}ms")
        
        print(f"\nObject Statistics:")
        print(f"  Promoted to old:  {stats['objects_promoted']}")
        print(f"  Collected:        {stats['objects_collected']}")
        print(f"  Current young:    {stats['current_young_objects']}")
        print(f"  Current old:      {stats['current_old_objects']}")
        
        print("="*70)


def demo_generational_gc():
    """Demonstrate generational GC"""
    print("🗑️  Generational Garbage Collector Demo\n")
    
    gc = GenerationalGC(young_threshold=100, old_threshold=500)
    
    # Create root objects (always reachable)
    root1 = gc.allocate("root_object_1")
    gc.add_root(root1)
    
    # Allocate many short-lived objects
    print("Allocating 500 short-lived objects...")
    for i in range(500):
        obj = gc.allocate(f"temp_{i}")
        # These will be garbage (not reachable from roots)
    
    # Allocate some long-lived objects
    print("Allocating 50 long-lived objects...")
    long_lived = []
    for i in range(50):
        obj = gc.allocate(f"long_lived_{i}")
        root1.references.append(obj)
        long_lived.append(obj)
    
    # More short-lived allocations
    print("Allocating 300 more short-lived objects...")
    for i in range(300):
        obj = gc.allocate(f"temp2_{i}")
    
    # Manual major GC
    if gc.should_collect_old():
        print("\nOld generation full - triggering major GC...")
        gc.collect_old()
    
    # Print statistics
    gc.print_statistics()
    
    # Show efficiency
    stats = gc.get_statistics()
    if stats['young_collections'] > 0:
        efficiency = (stats['young_collections'] / 
                     (stats['young_collections'] + stats['old_collections']) * 100)
        print(f"\n✅ Minor GC efficiency: {efficiency:.1f}%")
        print(f"   (Most collections were fast minor GCs!)")


if __name__ == '__main__':
    demo_generational_gc()
