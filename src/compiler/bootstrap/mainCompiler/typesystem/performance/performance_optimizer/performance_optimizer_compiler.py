#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("OptimizationLevel", ['None', 'Basic', 'Aggressive', 'Maximum'], "OptimizationLevel definition"),
    ("TargetArchitecture", ['X86_64', 'ARM64', 'RISCV64', 'WASM'], "TargetArchitecture definition"),
    ("OptimizationError", ['CacheError(String)', 'ParallelizationError(String)', 'CompilationError(String)', 'InternalError(String)'], "OptimizationError definition"),
]

@dataclass
class PerformanceOptimizerCompilerGenerator:
    """Python Enum ve C Tag üretici."""
    cases: List[Tuple[str, List[str], str]] = field(default_factory=lambda: CASES)
    tag_map: Dict[str, int] = field(default_factory=dict)

    def compile(self):
        offset = 0
        for group_name, variants, _ in self.cases:
            for i, var in enumerate(variants):
                self.tag_map[f"{group_name}::{var}"] = offset + i
            offset += len(variants)
        return self

    def emit_python_enums(self) -> str:
        output = "#!/usr/bin/env python3\nfrom enum import IntEnum\nfrom dataclasses import dataclass, field\nfrom typing import List, Optional, Any\n\n"
        output += "\n@dataclass\nclass CacheConfig:\n    pass\n\n@dataclass\nclass CachingSystem:\n    pass\n\n@dataclass\nclass CompileTimeConfig:\n    pass\n\n@dataclass\nclass CompileTimeOptimizer:\n    pass\n\n@dataclass\nclass HotPathDetector:\n    pass\n\n@dataclass\nclass IncrementalConfig:\n    pass\n\n@dataclass\nclass InlineConfig:\n    pass\n\n@dataclass\nclass JITConfig:\n    pass\n\n@dataclass\nclass LazyConfig:\n    pass\n\n@dataclass\nclass MonomorphizationConfig:\n    pass\n\n@dataclass\nclass OptimizedAST:\n    pass\n\n@dataclass\nclass OptimizedProgram:\n    pass\n\n@dataclass\nclass OptimizerConfig:\n    pass\n\n@dataclass\nclass ParallelConfig:\n    pass\n\n@dataclass\nclass ParallelProcessor:\n    pass\n\n@dataclass\nclass PerformanceImprovements:\n    pass\n\n@dataclass\nclass RuntimeConfig:\n    pass\n\n@dataclass\nclass RuntimeOptimizer:\n    pass\n\n@dataclass\nclass SelectiveJITCompiler:\n    pass\n\n@dataclass\nclass TypeSystemOptimizer:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_PERFORMANCE_OPTIMIZER_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_PERFORMANCE_OPTIMIZER_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = PerformanceOptimizerCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
