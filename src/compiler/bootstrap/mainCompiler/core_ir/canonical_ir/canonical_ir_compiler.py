#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("CoreIR", ['Func {\n        name:      String,\n        params:    Vec<IRParam>,\n        return_ty: IRType,\n        body:      Vec<CoreIR>,\n    }', 'When {\n        cond:      Box<CoreIR>,\n        then_blk:  Vec<CoreIR>,\n        else_blk:  Option<Vec<CoreIR>>,\n    }', 'Loop {\n        cond:      Box<CoreIR>,\n        body:      Vec<CoreIR>,\n    }', 'Give(Option<Box<CoreIR>>)', 'BinOp {\n        lhs: Box<CoreIR>,\n        op:  IROp,\n        rhs: Box<CoreIR>,\n        ty:  IRType,\n    }', 'UnOp {\n        op:  IROp,\n        val: Box<CoreIR>,\n        ty:  IRType,\n    }', 'Call {\n        target: String,\n        args:   Vec<CoreIR>,\n        ty:     IRType,\n    }', 'Load(String, IRType)', 'Store(String, Box<CoreIR>)', 'Literal(IRLit)', 'Alloc(IRType)', 'ComputeBlock {\n        device: String,\n        body:   Vec<CoreIR>,\n    }'], "CoreIR definition"),
    ("IRType", ['Void', 'Bool', 'I64', 'F64', 'Str', 'Ptr(Box<IRType>)', 'Vec(Box<IRType>)', 'Tensor(Vec<i64>, Box<IRType>)', 'Shape(String)'], "IRType definition"),
    ("IRLit", ['Bool(bool)', 'I64(i64)', 'F64(f64)', 'Str(String)'], "IRLit definition"),
    ("IROp", ['Add', 'Sub', 'Mul', 'Div', 'Mod', 'Eq', 'Ne', 'Lt', 'Gt', 'Le', 'Ge', 'And', 'Or', 'Not', 'BitAnd', 'BitOr', 'BitXor'], "IROp definition"),
]

@dataclass
class CanonicalIrCompilerGenerator:
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
        output += "\n@dataclass\nclass IRParam:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_CANONICAL_IR_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_CANONICAL_IR_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = CanonicalIrCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
