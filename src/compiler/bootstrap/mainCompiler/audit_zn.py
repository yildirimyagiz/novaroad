import os
import re

SOURCE_ROOT = "/Users/os2026/Downloads/novaRoad/nova/zn/src/compiler"
BOOTSTRAP_ROOT = "/Users/os2026/Downloads/novaRoad/nova/src/compiler/bootstrap/mainCompiler"


def clean_name(s):
    # Strip payloads like (i64) or {field: Type}
    return s.split("(")[0].split("{")[0].strip()


def extract_blocks(content, keyword):
    blocks = []
    pattern = rf"expose\s+{keyword}\s+(\w+)(?:\s+derives\s*\[.*?\])?"
    for match in re.finditer(pattern, content):
        name = match.group(1)
        start_index = content.find("{", match.end())
        if start_index == -1:
            continue
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
            blocks.append((name, block_content.strip()))
    return blocks


def parse_zn(file_path):
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
    except Exception:
        return set(), {}

    structs = set(re.findall(r"expose\s+data\s+(\w+)", content))
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


def get_compiler_cases(compiler_path):
    if not os.path.exists(compiler_path):
        return None
    try:
        with open(compiler_path, "r", encoding="utf-8") as f:
            content = f.read()
    except:
        return {}

    # Better parsing: find entries in CASES list
    results = {}
    # Use re.findall to get each Tuple[str, List[str], str] entry
    entries = re.findall(
        r"\s+\([\"'](\w+)[\"'],\s*\[(.*?)\],\s*[\"']", content, re.DOTALL
    )

    for name, variants_str in entries:
        # Re-check brackets for variants_str to handle nested [ ] correctly
        # Actually in _compiler.py, it's ['...'] so the comma and next quote are the boundaries

        vars_list = []
        # Match each quoted variant string
        for v in re.findall(r"[\"'](.*?)[\"']", variants_str, re.DOTALL):
            vars_list.append(v.strip())
        results[name] = vars_list
    return results


def main():
    report = "# 🔍 Nova Bootstrap Audit Report\n\n"
    report += f"**Oluşturulma Tarihi:** {os.popen('date').read().strip()}\n\n"
    report += "Bu rapor, `.zn` kaynak dosyaları ile bootstrap `_compiler.py` dosyaları arasındaki senkronizasyonu kontrol eder.\n\n"

    stats = {
        "total_zn": 0,
        "missing_modules": 0,
        "missing_enums": 0,
        "missing_variants": 0,
    }

    for dirpath, dirnames, filenames in os.walk(SOURCE_ROOT):
        dirnames[:] = [d for d in dirnames if not d.startswith(".")]
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

            options = [(m_rel, m_name)]
            if rel_path.startswith("compiler/"):
                sub_rel = rel_path[len("compiler/") :]
                if f == "mod.zn":
                    options.append(
                        (
                            os.path.dirname(sub_rel),
                            os.path.basename(os.path.dirname(sub_rel)),
                        )
                    )
                else:
                    options.append((sub_rel[:-3], f[:-3]))

            # Handle collision guard suffix if needed
            forbidden = {
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

            compiler_file = None
            found_rel = None
            for r, n in options:
                # Try original name
                tp = os.path.join(BOOTSTRAP_ROOT, r, f"{n}_compiler.py")
                if os.path.exists(tp):
                    compiler_file = tp
                    found_rel = r
                    break
                # Try with _mod if parts match forbidden
                r_parts = r.split(os.sep)
                r_mod = os.sep.join(
                    [p + "_mod" if p in forbidden else p for p in r_parts]
                )
                n_mod = n + "_mod" if n in forbidden else n
                tp_mod = os.path.join(BOOTSTRAP_ROOT, r_mod, f"{n_mod}_compiler.py")
                if os.path.exists(tp_mod):
                    compiler_file = tp_mod
                    found_rel = r_mod
                    break

            structs, enums = parse_zn(zn_path)
            if not enums and not structs:
                continue

            stats["total_zn"] += 1

            if compiler_file is None:
                report += f"### ❌ Missing Module: `{rel_path}`\n"
                stats["missing_modules"] += 1
                continue

            compiler_cases = get_compiler_cases(compiler_file)
            module_header = False

            for enum_name, variants in enums.items():
                if enum_name not in compiler_cases:
                    if not module_header:
                        report += (
                            f"### 📂 Module: `{rel_path}` (Found in `{found_rel}`)\n"
                        )
                        module_header = True
                    report += f"- [ ] Missing Enum: `{enum_name}`\n"
                    stats["missing_enums"] += 1
                else:
                    c_variants = [clean_name(v) for v in compiler_cases[enum_name]]
                    missing_vars = []
                    for v in variants:
                        v_clean = clean_name(v)
                        if v_clean not in c_variants:
                            missing_vars.append(v)

                    if missing_vars:
                        if not module_header:
                            report += f"### 📂 Module: `{rel_path}` (Found in `{found_rel}`)\n"
                            module_header = True
                        report += f"- [ ] Missing Variants in `{enum_name}`: {', '.join(missing_vars)}\n"
                        stats["missing_variants"] += len(missing_vars)

            if module_header:
                report += "\n"

    report += f"\n## 📈 Statistics\n"
    report += f"- Toplam taranan `.zn` dosyası: {stats['total_zn']}\n"
    report += f"- Eksik modül yapısı: {stats['missing_modules']}\n"
    report += f"- Eksik Enum tanımları: {stats['missing_enums']}\n"
    report += f"- Eksik Varyant tanımları: {stats['missing_variants']}\n"

    output_path = os.path.join(BOOTSTRAP_ROOT, "BOOTSTRAP_AUDIT_PLAN.md")
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(report)
    print(f"✅ Audit complete. Stats: {stats}")


if __name__ == "__main__":
    main()
