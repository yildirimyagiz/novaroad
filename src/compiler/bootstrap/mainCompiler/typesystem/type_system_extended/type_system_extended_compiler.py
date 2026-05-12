#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("TypeKindExtended", ['I32', 'I64', 'F32', 'F64', 'Bool', 'Str', 'Void', 'Unknown', 'Array(Box<TypeKindExtended>)', 'Struct(String)', 'Function {\n        params: Vec<TypeKindExtended>,\n        ret: Box<TypeKindExtended>,\n        generics: Vec<String>,\n    }', 'Generic(String)', 'Pi {\n        param_name: String,\n        param_type: Box<TypeKindExtended>,\n        body_type: Box<TypeKindExtended>,\n    }', 'Sigma {\n        fst_name: String,\n        fst_type: Box<TypeKindExtended>,\n        snd_type: Box<TypeKindExtended>,\n    }', 'Equality {\n        base_type: Box<TypeKindExtended>,\n        lhs: ValueExpr,\n        rhs: ValueExpr,\n    }', 'Refinement {\n        base_type: Box<TypeKindExtended>,\n        predicate: ValueExpr,\n    }', 'Universe(u64)', 'Meta(u64)', 'MobileResource {\n        resource_type: MobileResourceType,\n        device_state: DeviceState,\n    }', 'BatteryDependent {\n        base_type: Box<TypeKindExtended>,\n        min_battery_level: f64,\n    }', 'ThermalDependent {\n        base_type: Box<TypeKindExtended>,\n        allowed_thermal_states: Vec<String>,\n    }', 'PermissionDependent {\n        base_type: Box<TypeKindExtended>,\n        required_permissions: Vec<String>,\n    }', 'PlatformDependent {\n        base_type: Box<TypeKindExtended>,\n        supported_platforms: Vec<String>,\n    }', 'CapabilityDependent {\n        base_type: Box<TypeKindExtended>,\n        required_capabilities: Vec<String>,\n    }'], "TypeKindExtended definition"),
]

@dataclass
class TypeSystemExtendedCompilerGenerator:
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
        output += "\n@dataclass\nclass TypeCheckerExtended:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_TYPE_SYSTEM_EXTENDED_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_TYPE_SYSTEM_EXTENDED_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = TypeSystemExtendedCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
