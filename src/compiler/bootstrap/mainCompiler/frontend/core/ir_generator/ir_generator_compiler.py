#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("IRInstruction", ['Const(IRConstant)', 'Add(IRValue, IRValue)', 'Sub(IRValue, IRValue)', 'Mul(IRValue, IRValue)', 'Div(IRValue, IRValue)', 'Rem(IRValue, IRValue)', 'Pow(IRValue, IRValue)', 'Eq(IRValue, IRValue)', 'Ne(IRValue, IRValue)', 'Lt(IRValue, IRValue)', 'Gt(IRValue, IRValue)', 'Le(IRValue, IRValue)', 'Ge(IRValue, IRValue)', 'And(IRValue, IRValue)', 'Or(IRValue, IRValue)', 'BitAnd(IRValue, IRValue)', 'BitOr(IRValue, IRValue)', 'BitXor(IRValue, IRValue)', 'Shl(IRValue, IRValue)', 'Shr(IRValue, IRValue)', 'Load(IRValue)', 'Store(IRValue, IRValue)', 'Alloca(IRType)', 'GetElementPtr(IRValue, Vec<IRValue>)', 'Phi(Vec<(IRValue, String)>)', 'Call(IRValue, Vec<IRValue>)', 'Bitcast(IRValue, IRType)', 'Trunc(IRValue, IRType)', 'Sext(IRValue, IRType)', 'Zext(IRValue, IRType)', 'TensorOp(TensorOperation, Vec<IRValue>)', 'FlowOp(FlowOperation, Vec<IRValue>)', 'UnitOp(UnitOperation, Vec<IRValue>)', 'Sync(SyncKind)', 'Launch(IRValue, Vec<IRValue>)', 'Grad(IRValue, IRValue)', 'Backprop(IRValue)'], "IRInstruction definition"),
    ("SyncKind", ['Threads', 'Memory', 'Device'], "SyncKind definition"),
    ("IRTerminator", ['Ret(Option<IRValue>)', 'Br(String)', 'CondBr(IRValue, String, String)', 'Switch(IRValue, String, Vec<(IRValue, String)>)', 'Unreachable'], "IRTerminator definition"),
    ("IRType", ['Void', 'Bool', 'Int8', 'Int16', 'Int32', 'Int64', 'Float32', 'Float64', 'Pointer(Box<IRType>)', 'Array(Box<IRType>, usize)', 'Struct(Vec<IRType>)', 'Function(Vec<IRType>, Box<IRType>)', 'Tensor(Box<IRType>, Vec<usize>)', 'Flow(FlowKind, Box<IRType>)', 'Quantity(Box<IRType>, String)'], "IRType definition"),
    ("IRConstant", ['Bool(bool)', 'Int(i64)', 'Float(f64)', 'Str(String)', 'Array(Vec<IRConstant>)', 'Struct(Vec<IRConstant>)', 'Null'], "IRConstant definition"),
    ("TensorOperation", ['MatMul', 'Transpose', 'Reshape', 'Slice', 'Concat', 'Reduce'], "TensorOperation definition"),
    ("FlowOperation", ['SignalCreate', 'StreamCreate', 'TaskSpawn', 'ChannelSend', 'ChannelRecv'], "FlowOperation definition"),
    ("UnitOperation", ['Convert', 'Add', 'Mul'], "UnitOperation definition"),
    ("IRError", ['UndefinedSymbol(String)', 'TypeMismatch(String)', 'UnsupportedOperation(String)', 'InvalidAST(String)'], "IRError definition"),
]

@dataclass
class IrGeneratorCompilerGenerator:
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
        output += "\n@dataclass\nclass IRBasicBlock:\n    pass\n\n@dataclass\nclass IRFunction:\n    pass\n\n@dataclass\nclass IRGenerator:\n    pass\n\n@dataclass\nclass IRGlobal:\n    pass\n\n@dataclass\nclass IRModule:\n    pass\n\n@dataclass\nclass IRParameter:\n    pass\n\n@dataclass\nclass IRValue:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_IR_GENERATOR_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_IR_GENERATOR_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = IrGeneratorCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
