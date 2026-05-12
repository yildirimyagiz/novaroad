#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("Instruction", ['Add { lhs: Value, rhs: Value, ty: IRType }', 'Sub { lhs: Value, rhs: Value, ty: IRType }', 'Mul { lhs: Value, rhs: Value, ty: IRType }', 'Div { lhs: Value, rhs: Value, ty: IRType }', 'Rem { lhs: Value, rhs: Value, ty: IRType }', 'And { lhs: Value, rhs: Value, ty: IRType }', 'Or { lhs: Value, rhs: Value, ty: IRType }', 'Xor { lhs: Value, rhs: Value, ty: IRType }', 'Shl { lhs: Value, rhs: Value, ty: IRType }', 'Shr { lhs: Value, rhs: Value, ty: IRType }', 'Eq { lhs: Value, rhs: Value }', 'Ne { lhs: Value, rhs: Value }', 'Lt { lhs: Value, rhs: Value }', 'Le { lhs: Value, rhs: Value }', 'Gt { lhs: Value, rhs: Value }', 'Ge { lhs: Value, rhs: Value }', 'Load { ptr: Value, ty: IRType }', 'Store { ptr: Value, value: Value }', 'Alloca { ty: IRType, count: usize }', 'Br { target: usize }', 'CondBr { cond: Value, true_bb: usize, false_bb: usize }', 'Ret { value: Option<Value> }', 'Unreachable', 'Call { func: String, args: Vec<Value>, ty: IRType }', 'Trunc { value: Value, to_ty: IRType }', 'ZExt { value: Value, to_ty: IRType }', 'SExt { value: Value, to_ty: IRType }', 'FPTrunc { value: Value, to_ty: IRType }', 'FPExt { value: Value, to_ty: IRType }', 'FPToUI { value: Value, to_ty: IRType }', 'FPToSI { value: Value, to_ty: IRType }', 'UIToFP { value: Value, to_ty: IRType }', 'SIToFP { value: Value, to_ty: IRType }', 'Bitcast { value: Value, to_ty: IRType }', 'ExtractElement { vec: Value, index: Value }', 'InsertElement { vec: Value, element: Value, index: Value }', 'ExtractValue { aggregate: Value, indices: Vec<usize> }', 'InsertValue { aggregate: Value, value: Value, indices: Vec<usize> }', 'Phi { incoming: Vec<(Value, usize)>, ty: IRType }', 'Select { cond: Value, true_val: Value, false_val: Value }'], "Instruction definition"),
]

@dataclass
class InstructionsCompilerGenerator:
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
        output += "\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_INSTRUCTIONS_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_INSTRUCTIONS_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = InstructionsCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
