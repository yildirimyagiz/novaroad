import os
import re
import shutil

SOURCE_ROOT = "/Users/os2026/Downloads/novaRoad/nova/zn/src/compiler"
BOOTSTRAP_ROOT = "/Users/os2026/Downloads/novaRoad/nova/src/compiler/bootstrap/mainCompiler/"

FORBIDDEN = {
    "ast",
    "io",
    "os",
    "sys",
    "re",
    "json",
    "math",
    "time",
    "dataclasses",
    "typing",
    "types",
    "inspect",
}


def extract_blocks(content, keyword):
    """Extract blocks starting with keyword followed by Name and { ... } handling nested braces."""
    blocks = []
    # Pattern: expose <keyword> Name [derives...] {
    pattern = rf"expose\s+{keyword}\s+(\w+)(?:\s+derives\s*\[.*?\])?"

    for match in re.finditer(pattern, content):
        name = match.group(1)
        start_index = content.find("{", match.end())
        if start_index == -1:
            continue

        # Match braces
        depth = 0
        end_index = -1
        for i in range(start_index, len(content)):
            if content[i] == "{":
                depth += 1
            elif content[i] == "}":
                depth -= 1
                if depth == 0:
                    end_index = i
                    break

        if end_index != -1:
            block_content = content[start_index + 1 : end_index]
            blocks.append((name, block_content))
    return blocks


def parse_zn(file_path):
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
    except:
        return set(), {}

    # Simple search for data names
    structs = set(re.findall(r"expose\s+data\s+(\w+)", content))

    # Use matching brace logic for enums
    enum_blocks = extract_blocks(content, "cases")

    enum_data = {}
    for name, variants_str in enum_blocks:
        clean_variants_str = re.sub(r"//.*", "", variants_str)

        raw_variants = []
        depth = 0
        p_depth = 0
        current = ""
        for char in clean_variants_str:
            if char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
            elif char == "(":
                p_depth += 1
            elif char == ")":
                p_depth -= 1

            if char == "," and depth == 0 and p_depth == 0:
                raw_variants.append(current.strip())
                current = ""
            else:
                current += char
        raw_variants.append(current.strip())

        variants = []
        for v in raw_variants:
            v_clean = v.strip()
            if v_clean:
                variants.append(v_clean)
        enum_data[name] = variants

    return structs, enum_data


def get_zn_modules():
    modules = {}
    for root, dirs, files in os.walk(SOURCE_ROOT):
        dirs[:] = [d for d in dirs if not d.startswith(".")]
        for f in files:
            if not f.endswith(".zn"):
                continue
            source_path = os.path.join(root, f)
            rel_source = os.path.relpath(source_path, SOURCE_ROOT)

            parts = rel_source[:-3].split(os.sep)
            clean_parts = [p + "_mod" if p in FORBIDDEN else p for p in parts]

            if f == "mod.zn":
                b_rel = os.path.dirname(os.sep.join(clean_parts))
                if not b_rel:
                    continue
                mod_name = os.path.basename(b_rel)
            else:
                b_rel = os.sep.join(clean_parts)
                mod_name = clean_parts[-1]

            if b_rel not in modules:
                modules[b_rel] = {"sources": [], "name": mod_name}
            modules[b_rel]["sources"].append(source_path)
    return modules


def generate_compiler_code(mod_name, all_enums, all_structs):
    display_name = mod_name.replace("_mod", "")
    class_name_prefix = "".join(x.capitalize() for x in display_name.split("_"))
    tag_prefix = display_name.upper()

    case_lines = []
    for name, variants in all_enums.items():
        case_lines.append(f'    ("{name}", {list(variants)}, "{name} definition"),')
    cases_str = "\n".join(case_lines)

    struct_defs = ""
    for s in sorted(list(all_structs)):
        struct_defs += f"\\n@dataclass\\nclass {s}:\\n    pass\\n"

    return f'''#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
{cases_str}
]

@dataclass
class {class_name_prefix}CompilerGenerator:
    """Python Enum ve C Tag üretici."""
    cases: List[Tuple[str, List[str], str]] = field(default_factory=lambda: CASES)
    tag_map: Dict[str, int] = field(default_factory=dict)

    def compile(self):
        offset = 0
        for group_name, variants, _ in self.cases:
            for i, var in enumerate(variants):
                self.tag_map[f"{{group_name}}::{{var}}"] = offset + i
            offset += len(variants)
        return self

    def emit_python_enums(self) -> str:
        output = "#!/usr/bin/env python3\\nfrom enum import IntEnum\\nfrom dataclasses import dataclass, field\\nfrom typing import List, Optional, Any\\n\\n"
        output += "{struct_defs}\\n"
        for group_name, variants, doc in self.cases:
            output += f'class {{group_name}}(IntEnum):\\n    \"\"\"{{doc}}\"\"\"\\n'
            for var in variants:
                tag = self.tag_map[f"{{group_name}}::{{var}}"]
                v_clean = var.split("(")[0].split("{{")[0].strip()
                output += f'    {{v_clean}} = {{tag}}\\n'
            output += "\\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_{tag_prefix}_TAGS_H"
        output = f"#ifndef {{guard}}\\n#define {{guard}}\\n\\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {{doc}} */\\n"
            for var in variants:
                tag = self.tag_map[f"{{group_name}}::{{var}}"]
                v_clean = var.split("(")[0].split("{{")[0].strip().upper()
                output += f"#define NOVA_{tag_prefix}_{{group_name.upper()}}_{{v_clean}} {{tag}}\\n"
            output += "\\n"
        output += "#endif\\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = {class_name_prefix}CompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
'''


def sync():
    modules = get_zn_modules()
    print(f"Syncing {len(modules)} modules...")

    keep = {
        "sync_and_clean.py",
        "master_bootstrap.py",
        "audit_zn.py",
        "rebuild_compilers.py",
    }
    for r in os.listdir(BOOTSTRAP_ROOT):
        path = os.path.join(BOOTSTRAP_ROOT, r)
        if r in keep:
            continue
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)

    for rel_path, info in modules.items():
        if "frontend/fixed" in rel_path:
            continue
        target_dir = os.path.join(BOOTSTRAP_ROOT, rel_path)
        os.makedirs(target_dir, exist_ok=True)

        combined_structs = set()
        combined_enums = {}
        for src in info["sources"]:
            s, e = parse_zn(src)
            combined_structs.update(s)
            for enum_name, variants in e.items():
                if enum_name not in combined_enums:
                    combined_enums[enum_name] = []
                for v in variants:
                    if v not in combined_enums[enum_name]:
                        combined_enums[enum_name].append(v)

        mod_name = info["name"]
        with open(os.path.join(target_dir, f"{mod_name}_compiler.py"), "w") as f:
            f.write(generate_compiler_code(mod_name, combined_enums, combined_structs))
        with open(os.path.join(target_dir, "__init__.py"), "w") as f:
            f.write(f"from .{mod_name} import *\nfrom .{mod_name}_ffi import *\n")

    print("✅ Turbo Sync Success.")


if __name__ == "__main__":
    sync()
