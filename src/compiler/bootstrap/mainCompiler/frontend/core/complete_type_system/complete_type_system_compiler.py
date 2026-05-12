#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("PrimitiveType", ['I8', 'I16', 'I32', 'I64', 'I128', 'U8', 'U16', 'U32', 'U64', 'U128', 'F16', 'F32', 'F64', 'Bool', 'Char', 'Str', 'Unit', 'Never'], "PrimitiveType definition"),
    ("GenericArg", ['Type(TypeExpr)', 'Const(ConstExpr)', 'Lifetime(Lifetime)'], "GenericArg definition"),
    ("ShapeDim", ['Named(String)', 'Const(i64)', 'Dynamic', 'Symbolic(String)'], "ShapeDim definition"),
    ("Effect", ['IO', 'Async', 'State(Box<TypeExpr>)', 'Exception', 'Nondet', 'Custom(String)'], "Effect definition"),
    ("ConstKind", ['Usize', 'I64', 'Bool'], "ConstKind definition"),
    ("SymbolicExpr", ['Var(String)', 'Add(Box<SymbolicExpr>, Box<SymbolicExpr>)', 'Mul(Box<SymbolicExpr>, Box<SymbolicExpr>)', 'Const(i64)'], "SymbolicExpr definition"),
    ("Platform", ['IOS', 'Android', 'Web', 'Desktop(DesktopOS)', 'Embedded'], "Platform definition"),
    ("DesktopOS", ['Windows', 'MacOS', 'Linux'], "DesktopOS definition"),
    ("TypeExpr", ['Prim(PrimitiveType)', 'Ref(RefType)', 'Generic(GenericType)', 'Path(PathType)', 'Tuple(TupleType)', 'Array(ArrayType)', 'Struct(StructType)', 'Enum(EnumType)', 'Qty(QtyType)', 'Tensor(TensorType)', 'Flow(FlowType)', 'FnPtr(FnPtrType)', 'DynRules(DynRulesType)', 'Union(UnionType)', 'Intersection(IntersectionType)', 'Phantom(PhantomType)', 'Effect(EffectType)', 'Lifetime(LifetimeType)', 'Const(ConstType)', 'Dependent(DependentType)', 'HigherKinded(HigherKindedType)', 'Opaque(OpaqueType)', 'Symbolic(SymbolicType)', 'Platform(PlatformType)', 'VPU(VPUType)', 'Infer(InferType)', 'Never(NeverType)', 'Any(AnyType)', 'Var(String)'], "TypeExpr definition"),
    ("ConstExpr", ['Int(i64)', 'Bool(bool)', 'Str(String)', 'Add(Box<ConstExpr>, Box<ConstExpr>)', 'Mul(Box<ConstExpr>, Box<ConstExpr>)', 'Var(String)'], "ConstExpr definition"),
]

@dataclass
class CompleteTypeSystemCompilerGenerator:
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
        output += "\n@dataclass\nclass AnyType:\n    pass\n\n@dataclass\nclass ArrayType:\n    pass\n\n@dataclass\nclass ConstType:\n    pass\n\n@dataclass\nclass DependentType:\n    pass\n\n@dataclass\nclass DimensionExpr:\n    pass\n\n@dataclass\nclass DynRulesType:\n    pass\n\n@dataclass\nclass EffectType:\n    pass\n\n@dataclass\nclass EnumType:\n    pass\n\n@dataclass\nclass FlowType:\n    pass\n\n@dataclass\nclass FnPtrType:\n    pass\n\n@dataclass\nclass GenericType:\n    pass\n\n@dataclass\nclass HigherKindedType:\n    pass\n\n@dataclass\nclass InferType:\n    pass\n\n@dataclass\nclass IntersectionType:\n    pass\n\n@dataclass\nclass Lifetime:\n    pass\n\n@dataclass\nclass LifetimeType:\n    pass\n\n@dataclass\nclass NeverType:\n    pass\n\n@dataclass\nclass OpaqueType:\n    pass\n\n@dataclass\nclass PathType:\n    pass\n\n@dataclass\nclass PhantomType:\n    pass\n\n@dataclass\nclass PlatformType:\n    pass\n\n@dataclass\nclass QtyType:\n    pass\n\n@dataclass\nclass RefType:\n    pass\n\n@dataclass\nclass StructType:\n    pass\n\n@dataclass\nclass SymbolicType:\n    pass\n\n@dataclass\nclass TensorType:\n    pass\n\n@dataclass\nclass TupleType:\n    pass\n\n@dataclass\nclass UnionType:\n    pass\n\n@dataclass\nclass VPUType:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_COMPLETE_TYPE_SYSTEM_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_COMPLETE_TYPE_SYSTEM_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = CompleteTypeSystemCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
