#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("WasmTarget", ['Browser', 'NodeJS', 'WASI', 'Emscripten'], "WasmTarget definition"),
    ("WasmFeature", ['MutableGlobals', 'NontrappingFPToInt', 'SignExtensionOps', 'MultiValue', 'BulkMemory', 'ReferenceTypes', 'SIMD128', 'RelaxedSIMD', 'Threads', 'ExceptionHandling', 'Memory64', 'ExtendedConst'], "WasmFeature definition"),
    ("WasmType", ['I32', 'I64', 'F32', 'F64', 'V128', 'FuncRef', 'ExternRef'], "WasmType definition"),
    ("ImportKind", ['Function(u32)', 'Table(TableType)', 'Memory(MemoryType)', 'Global(WasmGlobalType)'], "ImportKind definition"),
    ("ExportKind", ['Function(u32)', 'Table(u32)', 'Memory(u32)', 'Global(u32)'], "ExportKind definition"),
    ("Instruction", ['Unreachable', 'Nop', 'Block(BlockType, Vec<Instruction>)', 'Loop(BlockType, Vec<Instruction>)', 'If(BlockType, Vec<Instruction>, Option<Vec<Instruction>>)', 'Br(u32)', 'BrIf(u32)', 'BrTable(Vec<u32>, u32)', 'Return', 'Call(u32)', 'CallIndirect(u32, u32)', 'Drop', 'Select', 'LocalGet(u32)', 'LocalSet(u32)', 'LocalTee(u32)', 'GlobalGet(u32)', 'GlobalSet(u32)', 'I32Load(MemArg)', 'I64Load(MemArg)', 'F32Load(MemArg)', 'F64Load(MemArg)', 'I32Load8S(MemArg)', 'I32Load8U(MemArg)', 'I32Load16S(MemArg)', 'I32Load16U(MemArg)', 'I64Load8S(MemArg)', 'I64Load8U(MemArg)', 'I64Load16S(MemArg)', 'I64Load16U(MemArg)', 'I64Load32S(MemArg)', 'I64Load32U(MemArg)', 'I32Store(MemArg)', 'I64Store(MemArg)', 'F32Store(MemArg)', 'F64Store(MemArg)', 'I32Store8(MemArg)', 'I32Store16(MemArg)', 'I64Store8(MemArg)', 'I64Store16(MemArg)', 'I64Store32(MemArg)', 'MemorySize', 'MemoryGrow', 'I32Const(i32)', 'I64Const(i64)', 'F32Const(f32)', 'F64Const(f64)', 'I32Eqz', 'I32Eq', 'I32Ne', 'I32LtS', 'I32LtU', 'I32GtS', 'I32GtU', 'I32LeS', 'I32LeU', 'I32GeS', 'I32GeU', 'I32Clz', 'I32Ctz', 'I32Popcnt', 'I32Add', 'I32Sub', 'I32Mul', 'I32DivS', 'I32DivU', 'I32RemS', 'I32RemU', 'I32And', 'I32Or', 'I32Xor', 'I32Shl', 'I32ShrS', 'I32ShrU', 'I32Rotl', 'I32Rotr', 'I64Eqz', 'I64Eq', 'I64Ne', 'I64LtS', 'I64LtU', 'I64GtS', 'I64GtU', 'I64LeS', 'I64LeU', 'I64GeS', 'I64GeU', 'I64Clz', 'I64Ctz', 'I64Popcnt', 'I64Add', 'I64Sub', 'I64Mul', 'I64DivS', 'I64DivU', 'I64RemS', 'I64RemU', 'I64And', 'I64Or', 'I64Xor', 'I64Shl', 'I64ShrS', 'I64ShrU', 'I64Rotl', 'I64Rotr', 'F32Eq', 'F32Ne', 'F32Lt', 'F32Gt', 'F32Le', 'F32Ge', 'F32Abs', 'F32Neg', 'F32Ceil', 'F32Floor', 'F32Trunc', 'F32Nearest', 'F32Sqrt', 'F32Add', 'F32Sub', 'F32Mul', 'F32Div', 'F32Min', 'F32Max', 'F32Copysign', 'F64Eq', 'F64Ne', 'F64Lt', 'F64Gt', 'F64Le', 'F64Ge', 'F64Abs', 'F64Neg', 'F64Ceil', 'F64Floor', 'F64Trunc', 'F64Nearest', 'F64Sqrt', 'F64Add', 'F64Sub', 'F64Mul', 'F64Div', 'F64Min', 'F64Max', 'F64Copysign', 'I32WrapI64', 'I32TruncF32S', 'I32TruncF32U', 'I32TruncF64S', 'I32TruncF64U', 'I64ExtendI32S', 'I64ExtendI32U', 'I64TruncF32S', 'I64TruncF32U', 'I64TruncF64S', 'I64TruncF64U', 'F32ConvertI32S', 'F32ConvertI32U', 'F32ConvertI64S', 'F32ConvertI64U', 'F32DemoteF64', 'F64ConvertI32S', 'F64ConvertI32U', 'F64ConvertI64S', 'F64ConvertI64U', 'F64PromoteF32', 'I32ReinterpretF32', 'I64ReinterpretF64', 'F32ReinterpretI32', 'F64ReinterpretI64', 'V128Load(MemArg)', 'V128Store(MemArg)', 'V128Const(u128)', 'I8x16Shuffle(Vec<u8>)', 'MemoryInit(u32)', 'DataDrop(u32)', 'MemoryCopy', 'MemoryFill', 'RefNull(WasmType)', 'RefIsNull', 'RefFunc(u32)', 'TableGet(u32)', 'TableSet(u32)', 'TableSize(u32)', 'TableGrow(u32)', 'TableFill(u32)', 'TableCopy(u32, u32)', 'TableInit(u32, u32)', 'ElemDrop(u32)'], "Instruction definition"),
    ("CodegenError", ['UnsupportedInstruction', 'UnsupportedType', 'FunctionNotFound(String)', 'CompilationFailed(String)'], "CodegenError definition"),
    ("ValidationError", ['UnsupportedVersion', 'InvalidTypeIndex', 'InvalidFunctionIndex', 'MalformedInstruction'], "ValidationError definition"),
]

@dataclass
class WasmCodegenCompilerGenerator:
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
        output += "\n@dataclass\nclass BlockType:\n    pass\n\n@dataclass\nclass CodegenContext:\n    pass\n\n@dataclass\nclass CustomSection:\n    pass\n\n@dataclass\nclass DataSegment:\n    pass\n\n@dataclass\nclass ElementSegment:\n    pass\n\n@dataclass\nclass Expression:\n    pass\n\n@dataclass\nclass FunctionBody:\n    pass\n\n@dataclass\nclass FunctionType:\n    pass\n\n@dataclass\nclass Limits:\n    pass\n\n@dataclass\nclass MemArg:\n    pass\n\n@dataclass\nclass MemoryConfig:\n    pass\n\n@dataclass\nclass MemoryType:\n    pass\n\n@dataclass\nclass TableType:\n    pass\n\n@dataclass\nclass WasmBackend:\n    pass\n\n@dataclass\nclass WasmExport:\n    pass\n\n@dataclass\nclass WasmFunction:\n    pass\n\n@dataclass\nclass WasmGlobal:\n    pass\n\n@dataclass\nclass WasmGlobalType:\n    pass\n\n@dataclass\nclass WasmImport:\n    pass\n\n@dataclass\nclass WasmModule:\n    pass\n\n@dataclass\nclass WasmVersion:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_WASM_CODEGEN_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_WASM_CODEGEN_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = WasmCodegenCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
