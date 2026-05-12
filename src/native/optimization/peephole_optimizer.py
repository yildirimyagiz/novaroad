#!/usr/bin/env python3
"""
Peephole Optimizer for Nova Bytecode
Optimizes small bytecode patterns
"""

from typing import List, Optional
from dataclasses import dataclass


@dataclass
class Instruction:
    """Bytecode instruction"""
    opcode: str
    operand: Optional[any] = None
    
    def __repr__(self):
        if self.operand is not None:
            return f"{self.opcode} {self.operand}"
        return self.opcode


class PeepholeOptimizer:
    """
    Peephole optimization on bytecode
    
    Patterns optimized:
    1. LOAD x, POP → (removed)
    2. LOAD 0, ADD → (removed)
    3. LOAD 1, MUL → (removed)
    4. LOAD x, LOAD x → LOAD x, DUP
    5. JUMP to next instruction → (removed)
    6. Constant arithmetic → precomputed
    """
    
    def __init__(self):
        self.optimizations_applied = 0
        
    def optimize(self, instructions: List) -> List:
        """Main optimization entry point"""
        optimized = []
        i = 0
        
        while i < len(instructions):
            # Try each optimization pattern
            inst = instructions[i]
            
            # Pattern 1: LOAD x, POP → nothing
            if self.match_load_pop(instructions, i):
                i += 2  # Skip both
                self.optimizations_applied += 1
                continue
            
            # Pattern 2: LOAD 0, ADD → nothing
            if self.match_load_zero_add(instructions, i):
                i += 2  # Skip both
                self.optimizations_applied += 1
                continue
            
            # Pattern 3: LOAD 1, MUL → nothing
            if self.match_load_one_mul(instructions, i):
                i += 2
                self.optimizations_applied += 1
                continue
            
            # Pattern 4: LOAD x, LOAD x → LOAD x, DUP
            if self.match_duplicate_load(instructions, i):
                optimized.append(instructions[i])
                optimized.append(Instruction('DUP'))
                i += 2
                self.optimizations_applied += 1
                continue
            
            # Pattern 5: Constant folding in bytecode
            if self.match_constant_arithmetic(instructions, i):
                result = self.fold_constant_arithmetic(instructions, i)
                if result:
                    optimized.append(result)
                    i += 3  # Skip LOAD, LOAD, OP
                    self.optimizations_applied += 1
                    continue
            
            # Pattern 6: JUMP to next instruction → remove
            if self.match_redundant_jump(instructions, i):
                i += 1  # Skip jump
                self.optimizations_applied += 1
                continue
            
            # Pattern 7: NOT, NOT → nothing
            if self.match_double_not(instructions, i):
                i += 2
                self.optimizations_applied += 1
                continue
            
            # No pattern matched - keep instruction
            optimized.append(inst)
            i += 1
        
        return optimized
    
    def match_load_pop(self, instructions: List, i: int) -> bool:
        """LOAD x, POP → removed"""
        if i + 1 >= len(instructions):
            return False
        
        inst1 = instructions[i]
        inst2 = instructions[i + 1]
        
        return (self.get_opcode(inst1) == 'LOAD_CONST' and
                self.get_opcode(inst2) == 'POP')
    
    def match_load_zero_add(self, instructions: List, i: int) -> bool:
        """LOAD 0, ADD → removed (adding zero is identity)"""
        if i + 1 >= len(instructions):
            return False
        
        inst1 = instructions[i]
        inst2 = instructions[i + 1]
        
        return (self.get_opcode(inst1) == 'LOAD_CONST' and
                self.get_operand(inst1) == 0 and
                self.get_opcode(inst2) == 'ADD')
    
    def match_load_one_mul(self, instructions: List, i: int) -> bool:
        """LOAD 1, MUL → removed (multiplying by 1 is identity)"""
        if i + 1 >= len(instructions):
            return False
        
        inst1 = instructions[i]
        inst2 = instructions[i + 1]
        
        return (self.get_opcode(inst1) == 'LOAD_CONST' and
                self.get_operand(inst1) == 1 and
                self.get_opcode(inst2) == 'MUL')
    
    def match_duplicate_load(self, instructions: List, i: int) -> bool:
        """LOAD x, LOAD x → LOAD x, DUP"""
        if i + 1 >= len(instructions):
            return False
        
        inst1 = instructions[i]
        inst2 = instructions[i + 1]
        
        return (self.get_opcode(inst1) == 'LOAD_CONST' and
                self.get_opcode(inst2) == 'LOAD_CONST' and
                self.get_operand(inst1) == self.get_operand(inst2))
    
    def match_constant_arithmetic(self, instructions: List, i: int) -> bool:
        """LOAD a, LOAD b, OP → LOAD result"""
        if i + 2 >= len(instructions):
            return False
        
        inst1 = instructions[i]
        inst2 = instructions[i + 1]
        inst3 = instructions[i + 2]
        
        return (self.get_opcode(inst1) == 'LOAD_CONST' and
                self.get_opcode(inst2) == 'LOAD_CONST' and
                self.get_opcode(inst3) in ['ADD', 'SUB', 'MUL', 'DIV'])
    
    def fold_constant_arithmetic(self, instructions: List, i: int):
        """Fold constant arithmetic operations"""
        a = self.get_operand(instructions[i])
        b = self.get_operand(instructions[i + 1])
        op = self.get_opcode(instructions[i + 2])
        
        try:
            if op == 'ADD':
                result = a + b
            elif op == 'SUB':
                result = a - b
            elif op == 'MUL':
                result = a * b
            elif op == 'DIV':
                if b == 0:
                    return None  # Cannot fold division by zero
                result = a // b if isinstance(a, int) and isinstance(b, int) else a / b
            else:
                return None
            
            return Instruction('LOAD_CONST', result)
        except Exception:
            return None
    
    def match_redundant_jump(self, instructions: List, i: int) -> bool:
        """JUMP to next instruction → removed"""
        if i + 1 >= len(instructions):
            return False
        
        inst = instructions[i]
        
        if self.get_opcode(inst) == 'JUMP':
            target = self.get_operand(inst)
            return target == i + 1
        
        return False
    
    def match_double_not(self, instructions: List, i: int) -> bool:
        """NOT, NOT → removed (double negation)"""
        if i + 1 >= len(instructions):
            return False
        
        inst1 = instructions[i]
        inst2 = instructions[i + 1]
        
        return (self.get_opcode(inst1) == 'NOT' and
                self.get_opcode(inst2) == 'NOT')
    
    def get_opcode(self, instruction) -> str:
        """Get opcode from instruction"""
        if isinstance(instruction, Instruction):
            return instruction.opcode
        elif isinstance(instruction, dict):
            return instruction.get('opcode', '')
        elif hasattr(instruction, 'opcode'):
            return instruction.opcode
        return str(instruction)
    
    def get_operand(self, instruction):
        """Get operand from instruction"""
        if isinstance(instruction, Instruction):
            return instruction.operand
        elif isinstance(instruction, dict):
            return instruction.get('operand')
        elif hasattr(instruction, 'operand'):
            return instruction.operand
        return None
    
    def get_statistics(self) -> dict:
        """Get optimization statistics"""
        return {
            'optimizations_applied': self.optimizations_applied
        }


def example_usage():
    """Example of peephole optimization"""
    
    # Example bytecode
    bytecode = [
        Instruction('LOAD_CONST', 10),
        Instruction('POP'),              # Will be removed (pattern 1)
        Instruction('LOAD_CONST', 5),
        Instruction('LOAD_CONST', 0),
        Instruction('ADD'),              # Will be removed (pattern 2)
        Instruction('LOAD_CONST', 2),
        Instruction('LOAD_CONST', 3),
        Instruction('ADD'),              # Will be folded to LOAD 5
        Instruction('LOAD_CONST', 7),
        Instruction('LOAD_CONST', 7),    # Will become LOAD 7, DUP
    ]
    
    optimizer = PeepholeOptimizer()
    optimized = optimizer.optimize(bytecode)
    
    print("Original bytecode:")
    for inst in bytecode:
        print(f"  {inst}")
    
    print(f"\nOptimized bytecode ({optimizer.optimizations_applied} optimizations):")
    for inst in optimized:
        print(f"  {inst}")


if __name__ == '__main__':
    example_usage()
