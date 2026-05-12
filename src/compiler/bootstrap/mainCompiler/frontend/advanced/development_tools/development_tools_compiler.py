#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("ChangeType", ['Modified', 'Added', 'Deleted', 'Renamed'], "ChangeType definition"),
    ("ReloadResult", ['Success(String)', 'Failure(String)', 'NotApplicable'], "ReloadResult definition"),
    ("ExecutionState", ['Running', 'Paused(BreakReason)', 'Terminated'], "ExecutionState definition"),
    ("BreakReason", ['BreakpointHit', 'StepOver', 'StepInto', 'StepOut', 'Exception', 'UserRequest'], "BreakReason definition"),
    ("DebugError", ['NotAttached', 'AttachFailed', 'DetachFailed', 'BreakpointError', 'ContinueFailed', 'PauseFailed', 'StepFailed', 'EvaluationFailed', 'InvalidExpression'], "DebugError definition"),
    ("ProfileError", ['ExportFailed', 'RecordingNotActive', 'InsufficientData'], "ProfileError definition"),
    ("DevError", ['BuildFailed', 'RunFailed', 'HotReloadFailed', 'DebugError'], "DevError definition"),
]

@dataclass
class DevelopmentToolsCompilerGenerator:
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
        output += "\n@dataclass\nclass AssetReloadHandler:\n    pass\n\n@dataclass\nclass Breakpoint:\n    pass\n\n@dataclass\nclass BuildConfig:\n    pass\n\n@dataclass\nclass CodeReloadHandler:\n    pass\n\n@dataclass\nclass CoverageAnalyzer:\n    pass\n\n@dataclass\nclass CoverageData:\n    pass\n\n@dataclass\nclass CoverageDataRaw:\n    pass\n\n@dataclass\nclass CoverageReport:\n    pass\n\n@dataclass\nclass DebugEvent:\n    pass\n\n@dataclass\nclass DebugException:\n    pass\n\n@dataclass\nclass DebugValue:\n    pass\n\n@dataclass\nclass Debugger:\n    pass\n\n@dataclass\nclass DevelopmentProfiler:\n    pass\n\n@dataclass\nclass DevelopmentStatus:\n    pass\n\n@dataclass\nclass EvalResult:\n    pass\n\n@dataclass\nclass FileCoverageReport:\n    pass\n\n@dataclass\nclass FunctionCall:\n    pass\n\n@dataclass\nclass FunctionInfo:\n    pass\n\n@dataclass\nclass HotReloadEngine:\n    pass\n\n@dataclass\nclass LNIde:\n    pass\n\n@dataclass\nclass LanguageServer:\n    pass\n\n@dataclass\nclass PerformanceSample:\n    pass\n\n@dataclass\nclass PerformanceSampleRaw:\n    pass\n\n@dataclass\nclass ProfilingReport:\n    pass\n\n@dataclass\nclass ProjectConfig:\n    pass\n\n@dataclass\nclass ReloadChange:\n    pass\n\n@dataclass\nclass ReloadRequest:\n    pass\n\n@dataclass\nclass ServerCapabilities:\n    pass\n\n@dataclass\nclass SourceFile:\n    pass\n\n@dataclass\nclass StackFrame:\n    pass\n\n@dataclass\nclass StackFrameRaw:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_DEVELOPMENT_TOOLS_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_DEVELOPMENT_TOOLS_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = DevelopmentToolsCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
