#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("CompilerError", ['Lexical(LexError)', 'Syntax(ParseError)', 'Semantic(SemanticError)', 'Type(TypeError)', 'Symbol(SymbolError)', 'CodeGen(CodeGenError)', 'Runtime(RuntimeError)'], "CompilerError definition"),
    ("SemanticErrorKind", ['UnreachableCode', 'UnusedVariable', 'UnusedFunction', 'MissingReturn', 'DeadCode', 'InfiniteLoop', 'ContractViolation', 'UnsafeOperation', 'DeprecatedUsage'], "SemanticErrorKind definition"),
    ("SymbolKind", ['Variable', 'Function', 'DataType', 'Trait', 'Module', 'Field', 'Method', 'Constant', 'TypeAlias', 'Macro'], "SymbolKind definition"),
    ("SymbolError", ['NotFound { name: String, span: Span }', 'AlreadyDefined { name: String, span: Span }', 'WrongKind { name: String, expected: SymbolKind, found: SymbolKind, span: Span }', 'NotAccessible { name: String, span: Span }', 'CircularReference { name: String, span: Span }'], "SymbolError definition"),
    ("CodeGenPhase", ['IRGeneration', 'Optimization', 'TargetCodeEmission', 'Linking'], "CodeGenPhase definition"),
    ("Severity", ['Error', 'Warning', 'Info', 'Hint'], "Severity definition"),
]

@dataclass
class ErrorsCompilerGenerator:
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
        output += "\n@dataclass\nclass CodeGenError:\n    pass\n\n@dataclass\nclass Diagnostic:\n    pass\n\n@dataclass\nclass ErrorReporter:\n    pass\n\n@dataclass\nclass LexError:\n    pass\n\n@dataclass\nclass LintWarning:\n    pass\n\n@dataclass\nclass ParseError:\n    pass\n\n@dataclass\nclass RuntimeError:\n    pass\n\n@dataclass\nclass SemanticError:\n    pass\n\n@dataclass\nclass TypeError:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_ERRORS_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_ERRORS_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = ErrorsCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
