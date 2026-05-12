#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("Nat", ['Zero', 'Succ(Box<Nat>)'], "Nat definition"),
    ("TBool", ['True', 'False'], "TBool definition"),
    ("TypeList", ['Nil', 'Cons(T, Box<TypeList<T>>)'], "TypeList definition"),
    ("TypeLevelValue", ['Nat(i64)', 'Bool(bool)', 'Str(String)', 'List(Vec<TypeLevelValue>)', 'Tuple(Vec<TypeLevelValue>)', 'Fn(String, Box<TypeLevelValue>)', 'Unit', 'Error(String)'], "TypeLevelValue definition"),
    ("TypeExpr", ['Prim(String)', 'Var(String)', 'App(Box<TypeExpr>, Vec<TypeExpr>)', 'Fn(Vec<TypeExpr>, Box<TypeExpr>)', 'Tuple(Vec<TypeExpr>)', 'Const(TypeLevelValue)', 'Add(Box<TypeExpr>, Box<TypeExpr>)', 'Sub(Box<TypeExpr>, Box<TypeExpr>)', 'Mul(Box<TypeExpr>, Box<TypeExpr>)', 'If(Box<TypeExpr>, Box<TypeExpr>, Box<TypeExpr>)'], "TypeExpr definition"),
    ("FileState", ['Closed', 'Opened', 'Locked'], "FileState definition"),
    ("FileMode", ['ReadOnly', 'WriteOnly', 'ReadWrite'], "FileMode definition"),
    ("FileError", ['NotFound(String)', 'PermissionDenied(String)', 'AlreadyOpen(String)', 'NotOpen', 'IoError(String)'], "FileError definition"),
]

@dataclass
class TypeLevelComputationCompilerGenerator:
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
        output += "\n@dataclass\nclass ConstRegistry:\n    pass\n\n@dataclass\nclass File:\n    pass\n\n@dataclass\nclass PhantomData:\n    pass\n\n@dataclass\nclass Refined:\n    pass\n\n@dataclass\nclass Sigma:\n    pass\n\n@dataclass\nclass SizedVec:\n    pass\n\n@dataclass\nclass TypeEnv:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_TYPE_LEVEL_COMPUTATION_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_TYPE_LEVEL_COMPUTATION_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = TypeLevelComputationCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
