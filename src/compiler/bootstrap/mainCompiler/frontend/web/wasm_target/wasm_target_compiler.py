#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("WASMOptimizationLevel", ['Debug', 'ReleaseSmall', 'ReleaseFast'], "WASMOptimizationLevel definition"),
    ("WASMInstruction", ['Unreachable', 'Nop', 'Block(String, Vec<WASMInstruction>)', 'Loop(String, Vec<WASMInstruction>)', 'If(String, Vec<WASMInstruction>, Option<Vec<WASMInstruction>>)', 'Br(i32)', 'BrIf(i32)', 'BrTable(Vec<i32>, i32)', 'Return', 'Call(i32)', 'CallIndirect(i32)', 'LocalGet(i32)', 'LocalSet(i32)', 'LocalTee(i32)', 'GlobalGet(i32)', 'GlobalSet(i32)', 'I32Load(i32, i32)', 'I64Load(i32, i32)', 'F32Load(i32, i32)', 'F64Load(i32, i32)', 'I32Store(i32, i32)', 'I64Store(i32, i32)', 'F32Store(i32, i32)', 'F64Store(i32, i32)', 'MemorySize', 'MemoryGrow', 'I32Const(i32)', 'I64Const(i64)', 'F32Const(f32)', 'F64Const(f64)', 'I32Add', 'I32Sub', 'I32Mul', 'I32DivS', 'I32DivU', 'I32RemS', 'I32RemU', 'I64Add', 'I64Sub', 'I64Mul', 'I64DivS', 'I64DivU', 'I64RemS', 'I64RemU', 'F32Add', 'F32Sub', 'F32Mul', 'F32Div', 'F64Add', 'F64Sub', 'F64Mul', 'F64Div', 'I32Eq', 'I32Ne', 'I32LtS', 'I32LtU', 'I32GtS', 'I32GtU', 'I32LeS', 'I32LeU', 'I32GeS', 'I32GeU', 'I64Eq', 'I64Ne', 'I64LtS', 'I64LtU', 'I64GtS', 'I64GtU', 'I64LeS', 'I64LeU', 'I64GeS', 'I64GeU', 'F32Eq', 'F32Ne', 'F32Lt', 'F32Gt', 'F32Le', 'F32Ge', 'F64Eq', 'F64Ne', 'F64Lt', 'F64Gt', 'F64Le', 'F64Ge'], "WASMInstruction definition"),
    ("WASMValueType", ['I32', 'I64', 'F32', 'F64'], "WASMValueType definition"),
    ("WASMExportKind", ['Function', 'Table', 'Memory', 'Global'], "WASMExportKind definition"),
    ("WASMImportKind", ['Function(WASMType)', 'Table(WASMTableType)', 'Memory(WASMMemoryType)', 'Global(WASMGlobalType)'], "WASMImportKind definition"),
    ("WASMElementType", ['FuncRef'], "WASMElementType definition"),
    ("WASMExportValue", ['Function(WASMFunctionRef)', 'Memory(WASMMemoryRef)', 'Global(WASMGlobalRef)'], "WASMExportValue definition"),
    ("WASMValue", ['I32(i32)', 'I64(i64)', 'F32(f32)', 'F64(f64)'], "WASMValue definition"),
    ("WASMError", ['CompilationFailed(String)', 'TypeMappingError(String)', 'CodeGenerationError(String)', 'ValidationError(String)'], "WASMError definition"),
]

@dataclass
class WasmTargetCompilerGenerator:
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
        output += "\n@dataclass\nclass WASMCodeGenerator:\n    pass\n\n@dataclass\nclass WASMDataSegment:\n    pass\n\n@dataclass\nclass WASMExport:\n    pass\n\n@dataclass\nclass WASMFunction:\n    pass\n\n@dataclass\nclass WASMFunctionRef:\n    pass\n\n@dataclass\nclass WASMFunctionTranslator:\n    pass\n\n@dataclass\nclass WASMFunctionType:\n    pass\n\n@dataclass\nclass WASMGlobal:\n    pass\n\n@dataclass\nclass WASMGlobalRef:\n    pass\n\n@dataclass\nclass WASMGlobalType:\n    pass\n\n@dataclass\nclass WASMImport:\n    pass\n\n@dataclass\nclass WASMInstance:\n    pass\n\n@dataclass\nclass WASMLimits:\n    pass\n\n@dataclass\nclass WASMMemory:\n    pass\n\n@dataclass\nclass WASMMemoryConfig:\n    pass\n\n@dataclass\nclass WASMMemoryRef:\n    pass\n\n@dataclass\nclass WASMMemoryType:\n    pass\n\n@dataclass\nclass WASMModule:\n    pass\n\n@dataclass\nclass WASMModuleBuilder:\n    pass\n\n@dataclass\nclass WASMRuntime:\n    pass\n\n@dataclass\nclass WASMTableType:\n    pass\n\n@dataclass\nclass WASMTarget:\n    pass\n\n@dataclass\nclass WASMType:\n    pass\n\n@dataclass\nclass WASMTypeMapper:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_WASM_TARGET_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_WASM_TARGET_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = WasmTargetCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
