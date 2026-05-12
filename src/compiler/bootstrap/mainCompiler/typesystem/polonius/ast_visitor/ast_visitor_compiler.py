#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("Expr", ['Block(Vec<Stmt>)', 'If(Box<Expr>, Box<Expr>, Option<Box<Expr>>)', 'Match(Box<Expr>, Vec<MatchArm>)', 'While(Box<Expr>, Box<Expr>)', 'For(String, Box<Expr>, Box<Expr>)', 'Let(String, Box<Expr>)', 'Assign(Box<Expr>, Box<Expr>)', 'Borrow(bool, Box<Expr>)', 'Call(Box<Expr>, Vec<Expr>)', 'Field(Box<Expr>, String)', 'Index(Box<Expr>, Box<Expr>)', 'Var(String)', 'Literal(Literal)'], "Expr definition"),
    ("Stmt", ['Expr(Expr)', 'Let(String, Expr)', 'Return(Expr)'], "Stmt definition"),
    ("Pattern", ['Wildcard', 'Var(String)', 'Literal(Literal)'], "Pattern definition"),
    ("Literal", ['Int(i64)', 'Bool(bool)', 'String(String)'], "Literal definition"),
    ("Type", ['Int', 'Bool', 'String', 'Unit'], "Type definition"),
]

@dataclass
class AstVisitorCompilerGenerator:
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
        output += "\n@dataclass\nclass ASTVisitor:\n    pass\n\n@dataclass\nclass FunctionParam:\n    pass\n\n@dataclass\nclass MatchArm:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_AST_VISITOR_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_AST_VISITOR_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = AstVisitorCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
