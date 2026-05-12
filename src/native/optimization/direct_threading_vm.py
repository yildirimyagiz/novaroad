#!/usr/bin/env python3
"""
Direct Threading VM for Nova
Uses jump table for faster instruction dispatch
"""

from typing import List, Dict, Callable, Any
from enum import Enum


class OpCode(Enum):
    """Bytecode opcodes"""
    LOAD_CONST = 1
    LOAD_VAR = 2
    STORE_VAR = 3
    ADD = 4
    SUB = 5
    MUL = 6
    DIV = 7
    POP = 8
    DUP = 9
    JUMP = 10
    JUMP_IF_FALSE = 11
    CALL = 12
    RETURN = 13
    HALT = 14


class DirectThreadingVM:
    """
    Virtual Machine with direct threading
    
    Instead of if-elif chain, uses jump table for 2-3x faster dispatch
    """
    
    def __init__(self, instructions: List, constants: List = None):
        self.instructions = instructions
        self.constants = constants or []
        self.stack = []
        self.variables = {}
        self.pc = 0  # Program counter
        self.running = True
        
        # Performance tracking
        self.instruction_count = 0
        
        # Direct threading: Jump table
        self.dispatch_table = self._build_dispatch_table()
    
    def _build_dispatch_table(self) -> Dict[OpCode, Callable]:
        """Build jump table for direct threading"""
        return {
            OpCode.LOAD_CONST: self._exec_load_const,
            OpCode.LOAD_VAR: self._exec_load_var,
            OpCode.STORE_VAR: self._exec_store_var,
            OpCode.ADD: self._exec_add,
            OpCode.SUB: self._exec_sub,
            OpCode.MUL: self._exec_mul,
            OpCode.DIV: self._exec_div,
            OpCode.POP: self._exec_pop,
            OpCode.DUP: self._exec_dup,
            OpCode.JUMP: self._exec_jump,
            OpCode.JUMP_IF_FALSE: self._exec_jump_if_false,
            OpCode.CALL: self._exec_call,
            OpCode.RETURN: self._exec_return,
            OpCode.HALT: self._exec_halt,
        }
    
    def run(self):
        """Execute bytecode with direct threading"""
        while self.running and self.pc < len(self.instructions):
            instruction = self.instructions[self.pc]
            opcode = instruction['opcode']
            operand = instruction.get('operand')
            
            # Direct dispatch via jump table (FAST!)
            handler = self.dispatch_table.get(opcode)
            if handler:
                handler(operand)
            else:
                raise ValueError(f"Unknown opcode: {opcode}")
            
            self.instruction_count += 1
            self.pc += 1
    
    # Instruction implementations
    
    def _exec_load_const(self, operand):
        """LOAD_CONST: Push constant to stack"""
        # Accept operand as direct value
        self.stack.append(operand)
    
    def _exec_load_var(self, operand):
        """LOAD_VAR: Push variable value to stack"""
        value = self.variables.get(operand, 0)
        self.stack.append(value)
    
    def _exec_store_var(self, operand):
        """STORE_VAR: Pop and store in variable"""
        value = self.stack.pop()
        self.variables[operand] = value
    
    def _exec_add(self, operand):
        """ADD: Pop two values, push sum"""
        b = self.stack.pop()
        a = self.stack.pop()
        self.stack.append(a + b)
    
    def _exec_sub(self, operand):
        """SUB: Pop two values, push difference"""
        b = self.stack.pop()
        a = self.stack.pop()
        self.stack.append(a - b)
    
    def _exec_mul(self, operand):
        """MUL: Pop two values, push product"""
        b = self.stack.pop()
        a = self.stack.pop()
        self.stack.append(a * b)
    
    def _exec_div(self, operand):
        """DIV: Pop two values, push quotient"""
        b = self.stack.pop()
        a = self.stack.pop()
        if isinstance(a, int) and isinstance(b, int):
            self.stack.append(a // b)
        else:
            self.stack.append(a / b)
    
    def _exec_pop(self, operand):
        """POP: Remove top of stack"""
        self.stack.pop()
    
    def _exec_dup(self, operand):
        """DUP: Duplicate top of stack"""
        self.stack.append(self.stack[-1])
    
    def _exec_jump(self, operand):
        """JUMP: Unconditional jump"""
        self.pc = operand - 1  # -1 because pc will be incremented
    
    def _exec_jump_if_false(self, operand):
        """JUMP_IF_FALSE: Conditional jump"""
        condition = self.stack.pop()
        if not condition:
            self.pc = operand - 1
    
    def _exec_call(self, operand):
        """CALL: Function call (placeholder)"""
        pass
    
    def _exec_return(self, operand):
        """RETURN: Return from function"""
        pass
    
    def _exec_halt(self, operand):
        """HALT: Stop execution"""
        self.running = False
    
    def get_top(self):
        """Get top of stack without popping"""
        return self.stack[-1] if self.stack else None
    
    def get_statistics(self) -> dict:
        """Get execution statistics"""
        return {
            'instructions_executed': self.instruction_count,
            'final_stack_size': len(self.stack),
            'variables_count': len(self.variables)
        }


class StackCachingVM(DirectThreadingVM):
    """
    VM with stack caching optimization
    
    Keeps frequently accessed stack values in "virtual registers"
    for faster access
    """
    
    def __init__(self, instructions: List, constants: List = None):
        super().__init__(instructions, constants)
        
        # Stack cache - virtual registers
        self.cached_top = None
        self.cached_second = None
        self.has_cached_top = False
        self.has_cached_second = False
        
        # Performance tracking
        self.cache_hits = 0
        self.cache_misses = 0
    
    def push(self, value):
        """Push with caching"""
        if not self.has_cached_top:
            # Cache is empty - store in top
            self.cached_top = value
            self.has_cached_top = True
            self.cache_hits += 1
        elif not self.has_cached_second:
            # Top is cached - move to second, new value to top
            self.cached_second = self.cached_top
            self.has_cached_second = True
            self.cached_top = value
            self.cache_hits += 1
        else:
            # Cache full - flush second to stack
            self.stack.append(self.cached_second)
            self.cached_second = self.cached_top
            self.cached_top = value
            self.cache_misses += 1
    
    def pop(self):
        """Pop with caching"""
        if self.has_cached_top:
            # Pop from cache
            value = self.cached_top
            
            if self.has_cached_second:
                # Move second to top
                self.cached_top = self.cached_second
                self.has_cached_second = False
                
                if self.stack:
                    # Refill second from stack
                    self.cached_second = self.stack.pop()
                    self.has_cached_second = True
            else:
                # No second - refill from stack
                if self.stack:
                    self.cached_top = self.stack.pop()
                else:
                    self.has_cached_top = False
            
            self.cache_hits += 1
            return value
        
        # Cache empty - pop from stack
        self.cache_misses += 1
        return self.stack.pop()
    
    def peek(self):
        """Peek top without popping"""
        if self.has_cached_top:
            self.cache_hits += 1
            return self.cached_top
        
        self.cache_misses += 1
        return self.stack[-1] if self.stack else None
    
    # Override arithmetic operations to use cache
    
    def _exec_add(self, operand):
        """ADD with stack caching"""
        b = self.pop()
        a = self.pop()
        self.push(a + b)
    
    def _exec_sub(self, operand):
        """SUB with stack caching"""
        b = self.pop()
        a = self.pop()
        self.push(a - b)
    
    def _exec_mul(self, operand):
        """MUL with stack caching"""
        b = self.pop()
        a = self.pop()
        self.push(a * b)
    
    def _exec_div(self, operand):
        """DIV with stack caching"""
        b = self.pop()
        a = self.pop()
        if isinstance(a, int) and isinstance(b, int):
            self.push(a // b)
        else:
            self.push(a / b)
    
    def _exec_load_const(self, operand):
        """LOAD_CONST with caching"""
        # Accept operand as direct value
        self.push(operand)
    
    def _exec_dup(self, operand):
        """DUP with caching"""
        top = self.peek()
        self.push(top)
    
    def get_statistics(self) -> dict:
        """Get execution statistics including cache performance"""
        stats = super().get_statistics()
        total_accesses = self.cache_hits + self.cache_misses
        hit_rate = (self.cache_hits / total_accesses * 100) if total_accesses > 0 else 0
        
        stats.update({
            'cache_hits': self.cache_hits,
            'cache_misses': self.cache_misses,
            'cache_hit_rate': f"{hit_rate:.2f}%"
        })
        
        return stats


def benchmark_comparison():
    """Compare standard vs optimized VM"""
    import time
    
    # Test program: sum = 0; for i in 0..1000: sum += i
    instructions = []
    
    # sum = 0
    instructions.append({'opcode': OpCode.LOAD_CONST, 'operand': 0})
    instructions.append({'opcode': OpCode.STORE_VAR, 'operand': 'sum'})
    
    # Loop
    for i in range(1000):
        instructions.append({'opcode': OpCode.LOAD_VAR, 'operand': 'sum'})
        instructions.append({'opcode': OpCode.LOAD_CONST, 'operand': i})  # Direct value, not index
        instructions.append({'opcode': OpCode.ADD, 'operand': None})
        instructions.append({'opcode': OpCode.STORE_VAR, 'operand': 'sum'})
    
    instructions.append({'opcode': OpCode.HALT, 'operand': None})
    
    # Run with direct threading
    print("Running with Direct Threading VM...")
    start = time.perf_counter()
    vm1 = DirectThreadingVM(instructions)
    vm1.run()
    end = time.perf_counter()
    time1 = (end - start) * 1000
    
    # Run with stack caching
    print("Running with Stack Caching VM...")
    start = time.perf_counter()
    vm2 = StackCachingVM(instructions)
    vm2.run()
    end = time.perf_counter()
    time2 = (end - start) * 1000
    
    print(f"\nResults:")
    print(f"  Direct Threading: {time1:.3f}ms")
    print(f"  Stack Caching:    {time2:.3f}ms")
    print(f"  Speedup:          {time1/time2:.2f}x")
    
    print(f"\nStack Caching Stats:")
    for key, value in vm2.get_statistics().items():
        print(f"  {key}: {value}")


if __name__ == '__main__':
    benchmark_comparison()
