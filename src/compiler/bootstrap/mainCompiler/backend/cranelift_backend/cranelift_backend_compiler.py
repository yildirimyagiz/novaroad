#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("OptLevel", ['None', 'Speed', 'SpeedAndSize'], "OptLevel definition"),
    ("Endianness", ['Little', 'Big'], "Endianness definition"),
    ("CallingConvention", ['SystemV', 'WindowsFastcall', 'Wasm'], "CallingConvention definition"),
    ("Instruction", ['Jump(String)', 'Branch(Value, String, String)', 'Return(Option<Value>)', 'Call(String, Vec<Value>)', 'Load(Value, CraneliftType)', 'Store(Value, Value)', 'GlobalLoad(String)', 'GlobalStore(String, Value)', 'IAdd(Value, Value)', 'ISub(Value, Value)', 'IMul(Value, Value)', 'IDiv(Value, Value)', 'FAdd(Value, Value)', 'FSub(Value, Value)', 'FMul(Value, Value)', 'FDiv(Value, Value)', 'ICmp(CmpOp, Value, Value)', 'FCmp(CmpOp, Value, Value)', 'SExt(Value, CraneliftType)', 'ZExt(Value, CraneliftType)', 'Trunc(Value, CraneliftType)', 'F32ToF64(Value)', 'F64ToF32(Value)', 'I32ToF64(Value)', 'F64ToI32(Value)', 'Phi(Vec<(Value, String)>)'], "Instruction definition"),
    ("CmpOp", ['Eq', 'Ne', 'Lt', 'Le', 'Gt', 'Ge'], "CmpOp definition"),
    ("CraneliftType", ['I8', 'I16', 'I32', 'I64', 'U8', 'U16', 'U32', 'U64', 'F32', 'F64', 'B1', 'Ptr(CraneliftType)', 'Array(CraneliftType, u32)', 'Struct(Vec<CraneliftType>)', 'Void'], "CraneliftType definition"),
    ("RelocationKind", ['Absolute32', 'Relative32', 'FunctionCall', 'DataLoad'], "RelocationKind definition"),
    ("SymbolKind", ['Function', 'Global', 'Local'], "SymbolKind definition"),
    ("SectionKind", ['ReadOnlyData', 'ReadWriteData', 'UninitializedData'], "SectionKind definition"),
    ("CodegenError", ['UnsupportedInstruction', 'TypeMismatch(String)', 'TargetNotSupported(String)', 'CompilationFailed(String)'], "CodegenError definition"),
]

@dataclass
class CraneliftBackendCompilerGenerator:
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
        output += "\n@dataclass\nclass BasicBlock:\n    pass\n\n@dataclass\nclass CodegenContext:\n    pass\n\n@dataclass\nclass CompiledFunction:\n    pass\n\n@dataclass\nclass CompiledModule:\n    pass\n\n@dataclass\nclass CraneliftBackend:\n    pass\n\n@dataclass\nclass CraneliftFunction:\n    pass\n\n@dataclass\nclass CraneliftGlobal:\n    pass\n\n@dataclass\nclass CraneliftModule:\n    pass\n\n@dataclass\nclass DataSection:\n    pass\n\n@dataclass\nclass DebugInfo:\n    pass\n\n@dataclass\nclass FunctionBody:\n    pass\n\n@dataclass\nclass FunctionSignature:\n    pass\n\n@dataclass\nclass LineEntry:\n    pass\n\n@dataclass\nclass LocalVar:\n    pass\n\n@dataclass\nclass Relocation:\n    pass\n\n@dataclass\nclass SymbolInfo:\n    pass\n\n@dataclass\nclass TargetConfig:\n    pass\n\n@dataclass\nclass Value:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_CRANELIFT_BACKEND_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_CRANELIFT_BACKEND_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = CraneliftBackendCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
