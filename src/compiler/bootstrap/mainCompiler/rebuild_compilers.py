import os
import re

SOURCE_ROOT = "/Users/os2026/Downloads/novaRoad/nova/zn/src/compiler"
BOOTSTRAP_ROOT = "/Users/os2026/Downloads/novaRoad/nova/src/compiler/bootstrap/mainCompiler"


def parse_zn(file_path):
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
    except:
        return [], {}
    structs = re.findall(r"expose\s+data\s+(\w+)", content)
    # Find enums and their content
    enums = re.findall(r"expose\s+cases\s+(\w+)\s*\{([^}]+)\}", content, re.DOTALL)

    enum_data = {}
    for name, variants_str in enums:
        lines = variants_str.split("\n")
        variants = []
        for line in lines:
            line = line.strip()
            if not line or line.startswith("//"):
                continue
            # Handle complex variants like Variant(Payload), or Variant{field: Type},
            # Match the word before any paren or brace or comma
            match = re.search(r"^(\w+)", line)
            if match:
                variants.append(match.group(1))
        enum_data[name] = variants
    return structs, enum_data


def generate_compiler_code(mod_name, enums):
    class_name = "".join(x.capitalize() for x in mod_name.split("_"))
    tag_prefix = mod_name.upper()

    case_list = []
    for name, variants in enums.items():
        case_list.append(f'    ("{name}", {variants}, "{name} enum definition")')
    cases_str = ",\n".join(case_list)

    return f"""#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

CASES: List[Tuple[str, List[str], str]] = [
{cases_str}
]

@dataclass
class {class_name}Compiler:
    def compile(self): return self
    def emit_python_enums(self) -> str:
        output = "#!/usr/bin/env python3\\nfrom enum import IntEnum\\n\\n"
        for name, variants, doc in CASES:
            output += f'class {{name}}(IntEnum):\\n    \"\"\"{{doc}}\"\"\"\\n'
            for i, v in enumerate(variants):
                # Ensure valid python identifier (no payloads)
                v_clean = v.split('(')[0].split('{{')[0].strip()
                output += f'    {{v_clean}} = {{i}}\\n'
            output += '\\n'
        return output
    def emit_c_header(self) -> str:
        output = "#ifndef NOVA_{tag_prefix}_TAGS_H\\n#define NOVA_{tag_prefix}_TAGS_H\\n\\n"
        for name, variants, doc in CASES:
            for i, v in enumerate(variants):
                v_clean = v.split('(')[0].split('{{')[0].strip().upper()
                output += f"#define NOVA_{tag_prefix}_{{name.upper()}}_{{v_clean}} {{i}}\\n"
            output += "\\n"
        output += "#endif\\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = {class_name}Compiler()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
"""


def generate_impl_code(mod_name, structs):
    class_name = "".join(x.capitalize() for x in mod_name.split("_"))
    struct_defs = ""
    for s in structs:
        struct_defs += f"\n@dataclass\nclass {s}:\n    pass\n"

    return f"""from dataclasses import dataclass
from typing import List, Optional

@dataclass
class {class_name}:
    pass
{struct_defs}
"""


def rebuild_all():
    print("🏗️  Rebuilding all bootstrap modules (cleaned variants)...")
    for dirpath, dirnames, filenames in os.walk(SOURCE_ROOT):
        for f in filenames:
            if not f.endswith(".zn"):
                continue
            zn_path = os.path.join(dirpath, f)
            rel_path = os.path.relpath(zn_path, SOURCE_ROOT)

            if f == "mod.zn":
                m_rel = os.path.dirname(rel_path)
                m_name = os.path.basename(m_rel)
                if not m_rel:
                    continue
            else:
                m_rel = rel_path[:-3]
                m_name = f[:-3]

            target_dir = os.path.join(BOOTSTRAP_ROOT, m_rel)
            os.makedirs(target_dir, exist_ok=True)

            structs, enums = parse_zn(zn_path)

            with open(os.path.join(target_dir, f"{m_name}_compiler.py"), "w") as f_out:
                f_out.write(generate_compiler_code(m_name, enums))

            with open(os.path.join(target_dir, f"{m_name}.py"), "w") as f_out:
                f_out.write(generate_impl_code(m_name, structs))

            if not os.path.exists(os.path.join(target_dir, "__init__.py")):
                with open(os.path.join(target_dir, "__init__.py"), "w") as f_out:
                    f_out.write(f"from .{m_name} import *\n")


if __name__ == "__main__":
    rebuild_all()
