#!/usr/bin/env python3
"""
Enhanced LLVM JIT Compiler for Nova
Compiles hot paths to native code at runtime
"""

from typing import Dict, List, Optional
import sys

try:
    from llvmlite import ir, binding
    LLVM_AVAILABLE = True
    
    # Initialize LLVM
    binding.initialize()
    binding.initialize_native_target()
    binding.initialize_native_asmprinter()
except ImportError:
    LLVM_AVAILABLE = False
    print("⚠️  llvmlite not available. Install: pip install llvmlite")


class EnhancedLLVMJIT:
    """
    Enhanced JIT compiler with LLVM backend
    
    Features:
    - Hot path compilation
    - Type-specialized code generation
    - LLVM optimization passes
    - Runtime code execution
    """
    
    def __init__(self, optimization_level: str = "2"):
        if not LLVM_AVAILABLE:
            raise RuntimeError("LLVM not available")
        
        self.optimization_level = optimization_level
        
        # Compiled functions cache
        self.compiled_functions: Dict[str, any] = {}
        
        # LLVM execution engine
        self.target = binding.Target.from_default_triple()
        self.target_machine = self.target.create_target_machine()
        
        # Statistics
        self.compilations = 0
        self.compilation_time_ms = 0.0
    
    def compile_function(self, func_name: str, bytecode: List, arg_types: List[type]) -> Optional[any]:
        """
        Compile function to native code
        
        Args:
            func_name: Function name
            bytecode: Function bytecode
            arg_types: Argument types for specialization
        
        Returns:
            Compiled native function
        """
        import time
        start = time.perf_counter()
        
        # Create LLVM module
        module = ir.Module(name=f"jit_{func_name}")
        
        # Generate function signature based on types
        llvm_arg_types = [self._type_to_llvm(t) for t in arg_types]
        func_type = ir.FunctionType(ir.IntType(64), llvm_arg_types)
        
        # Create function
        func = ir.Function(module, func_type, name=func_name)
        
        # Create entry block
        block = func.append_basic_block(name="entry")
        builder = ir.IRBuilder(block)
        
        # Compile bytecode to LLVM IR
        self._compile_bytecode_to_ir(builder, bytecode, func.args)
        
        # Return 0 (placeholder)
        builder.ret(ir.Constant(ir.IntType(64), 0))
        
        # Optimize module
        self._optimize_module(module)
        
        # Compile to machine code
        try:
            llvm_ir = str(module)
            mod = binding.parse_assembly(llvm_ir)
            mod.verify()
            
            # Create execution engine
            target_machine = self.target.create_target_machine()
            
            # Compile
            engine = binding.create_mcjit_compiler(mod, target_machine)
            engine.finalize_object()
            
            # Get function pointer
            func_ptr = engine.get_function_address(func_name)
            
            # Cache compiled function
            self.compiled_functions[func_name] = func_ptr
            
            # Update stats
            self.compilations += 1
            end = time.perf_counter()
            self.compilation_time_ms += (end - start) * 1000
            
            print(f"✅ JIT compiled: {func_name} ({(end-start)*1000:.2f}ms)")
            
            return func_ptr
            
        except Exception as e:
            print(f"❌ JIT compilation failed for {func_name}: {e}")
            return None
    
    def _type_to_llvm(self, py_type: type) -> ir.Type:
        """Convert Python type to LLVM type"""
        if py_type == int:
            return ir.IntType(64)
        elif py_type == float:
            return ir.DoubleType()
        elif py_type == bool:
            return ir.IntType(1)
        else:
            return ir.IntType(64)  # Default to i64
    
    def _compile_bytecode_to_ir(self, builder: ir.IRBuilder, bytecode: List, args: List):
        """
        Compile bytecode to LLVM IR
        
        This is a simplified example. Production version would:
        - Handle all opcodes
        - Manage SSA properly
        - Optimize control flow
        """
        
        # Example: Simple arithmetic compilation
        # This would be much more complex in production
        
        # For demo: Just return sum of arguments
        if len(args) >= 2:
            result = builder.add(args[0], args[1])
            # Store result somewhere (simplified)
        
        # Real implementation would process each bytecode instruction
    
    def _optimize_module(self, module: ir.Module):
        """
        Run LLVM optimization passes
        
        Optimizations:
        - Constant folding
        - Dead code elimination
        - Inlining
        - Loop unrolling
        - Vectorization
        """
        
        if not LLVM_AVAILABLE:
            return
        
        # Parse module
        llvm_ir = str(module)
        llvm_module = binding.parse_assembly(llvm_ir)
        
        # Create pass manager
        pmb = binding.PassManagerBuilder()
        
        # Set optimization level
        if self.optimization_level == "0":
            pmb.opt_level = 0
        elif self.optimization_level == "1":
            pmb.opt_level = 1
        elif self.optimization_level == "2":
            pmb.opt_level = 2
        elif self.optimization_level == "3":
            pmb.opt_level = 3
        else:
            pmb.opt_level = 2
        
        # Create module pass manager
        pm = binding.ModulePassManager()
        pmb.populate(pm)
        
        # Run optimizations
        pm.run(llvm_module)
    
    def get_statistics(self) -> dict:
        """Get JIT statistics"""
        return {
            'compilations': self.compilations,
            'total_compile_time_ms': self.compilation_time_ms,
            'avg_compile_time_ms': (self.compilation_time_ms / self.compilations 
                                   if self.compilations > 0 else 0),
            'cached_functions': len(self.compiled_functions)
        }
    
    def print_statistics(self):
        """Print JIT statistics"""
        stats = self.get_statistics()
        
        print("\n" + "="*70)
        print("LLVM JIT STATISTICS")
        print("="*70)
        
        print(f"\nCompilations: {stats['compilations']}")
        print(f"Total compile time: {stats['total_compile_time_ms']:.2f}ms")
        print(f"Avg compile time: {stats['avg_compile_time_ms']:.2f}ms")
        print(f"Cached functions: {stats['cached_functions']}")
        
        print("="*70)


def demo_enhanced_jit():
    """Demonstrate LLVM JIT compilation"""
    
    if not LLVM_AVAILABLE:
        print("❌ LLVM not available. Install llvmlite to run this demo.")
        return
    
    print("🚀 Enhanced LLVM JIT Demo\n")
    
    jit = EnhancedLLVMJIT(optimization_level="2")
    
    # Example bytecode for simple add function
    add_bytecode = [
        {'opcode': 'LOAD_ARG', 'operand': 0},
        {'opcode': 'LOAD_ARG', 'operand': 1},
        {'opcode': 'ADD', 'operand': None},
        {'opcode': 'RETURN', 'operand': None},
    ]
    
    # Compile for int+int
    print("Compiling add(int, int)...")
    jit.compile_function("add_int", add_bytecode, [int, int])
    
    # Compile for float+float
    print("Compiling add(float, float)...")
    jit.compile_function("add_float", add_bytecode, [float, float])
    
    # Print statistics
    jit.print_statistics()


if __name__ == '__main__':
    demo_enhanced_jit()
