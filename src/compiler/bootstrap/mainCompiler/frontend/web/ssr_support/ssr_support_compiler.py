#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("HydrationResult", ['Success(String)', 'Failed(String)', 'Skipped(String)', 'Timeout(String)'], "HydrationResult definition"),
    ("StreamChunk", ['Html(String)', 'ComponentData(String, HashMap<String, String>)', 'Script(String)', 'Style(String)', 'End'], "StreamChunk definition"),
    ("MiddlewareError", ['RequestProcessingFailed(String)', 'ResponseProcessingFailed(String)'], "MiddlewareError definition"),
    ("SSRError", ['ComponentRenderFailed(String)', 'HydrationFailed(String)', 'RoutingError(String)', 'TemplateError(String)'], "SSRError definition"),
]

@dataclass
class SsrSupportCompilerGenerator:
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
        output += "\n@dataclass\nclass Cookie:\n    pass\n\n@dataclass\nclass HydrationConfig:\n    pass\n\n@dataclass\nclass HydrationEngine:\n    pass\n\n@dataclass\nclass MetaTag:\n    pass\n\n@dataclass\nclass SSGBuildResult:\n    pass\n\n@dataclass\nclass SSGConfig:\n    pass\n\n@dataclass\nclass SSGPage:\n    pass\n\n@dataclass\nclass SSRCache:\n    pass\n\n@dataclass\nclass SSRContext:\n    pass\n\n@dataclass\nclass SSREngine:\n    pass\n\n@dataclass\nclass SSRMetrics:\n    pass\n\n@dataclass\nclass ServerRequest:\n    pass\n\n@dataclass\nclass ServerResponse:\n    pass\n\n@dataclass\nclass ServerRouter:\n    pass\n\n@dataclass\nclass StreamingRenderer:\n    pass\n\n@dataclass\nclass StreamingSSRConfig:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_SSR_SUPPORT_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_SSR_SUPPORT_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = SsrSupportCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
