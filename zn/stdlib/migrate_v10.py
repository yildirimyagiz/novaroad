import os
import re


def migrate_file(filepath):
    if os.path.islink(filepath):
        return
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return

    # Step 1: Global string replacements for keywords
    content = re.sub(r"\bpub\b", "open", content)
    content = re.sub(r"\bmod\b", "space", content)
    content = re.sub(r"\bbring\b", "use", content)
    content = re.sub(r"\bbehave\b", "rules", content)
    content = re.sub(r"\bapply\b", "impl", content)
    content = re.sub(r"\bif\b", "check", content)
    content = re.sub(r"\bmut\b", "var", content)
    content = re.sub(r"\benum\b", "cases", content)
    content = re.sub(r"\btrait\b", "rules", content)
    content = re.sub(r"\btype\b", "alias", content)

    content = content.replace("&mut", "&var").replace("*mut", "&var")
    content = re.sub(r"let\s+var", "var", content)
    content = re.sub(r"\breturn\b", "yield", content)
    content = re.sub(r"panic!\(", "abort(", content)
    content = re.sub(r"assert!\(", "assert(", content)
    content = re.sub(r"format!\(", "format(", content)
    content = re.sub(r"vec!\[(.*?)\]", r"[\1]", content)

    # Step 2: Loops
    content = re.sub(
        r"\bfor\s+([a-zA-Z_][a-zA-Z0-9_]*)\s+in\s+", r"each \1 in ", content
    )
    content = re.sub(
        r"\bfor\s+\(([^)]+)\)\s+in\s+([^\{]+)\{",
        r"each entry in \2 { let (\1) = entry;",
        content,
    )

    # Step 3: Yield injection for block-end expressions
    def inject_yield_multi(match):
        prefix = match.group(1)
        expr = match.group(2)
        inner = match.group(3)
        suffix = match.group(4)
        if any(
            expr.startswith(x)
            for x in [
                "yield ",
                "let ",
                "var ",
                "check ",
                "match ",
                "each ",
                "for ",
                "while ",
                "loop ",
            ]
        ):
            return match.group(0)
        return f"{prefix}yield {expr}{inner}{suffix}"

    content = re.sub(
        r"(\n\s*)([A-Z][a-zA-Z0-9_]*\(|Ok\(|Err\(|Result::|Some\(|None|\[|\()([^;\n]+?)\s*(\n\s*\})",
        inject_yield_multi,
        content,
    )

    def inject_yield_single(match):
        prefix = match.group(1)
        expr = match.group(2)
        inner = match.group(3)
        suffix = match.group(4)
        if any(
            expr.startswith(x)
            for x in [
                "yield ",
                "let ",
                "var ",
                "check ",
                "match ",
                "each ",
                "for ",
                "while ",
                "loop ",
            ]
        ):
            return match.group(0)
        return f"{prefix}yield {expr}{inner}{suffix}"

    content = re.sub(
        r"(\{\s*)([A-Z][a-zA-Z0-9_]*\(|Ok\(|Err\(|Some\(|None|\[|\()([^\}\n]+?)\s*(\})",
        inject_yield_single,
        content,
    )

    # Step 4: Redundancy cleanup
    content = re.sub(r"\bopen\s+open\b", "open", content)
    content = re.sub(r"\bvar\s+var\b", "var", content)
    content = re.sub(r"\bopen\s+cases\b", "open cases", content)

    try:
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(content)
    except Exception as e:
        print(f"Error writing {filepath}: {e}")


def walk_and_migrate(directory):
    for root, dirs, files in os.walk(directory):
        if any(x in root for x in ["ml2_backup", "ml3_backup", "old", "ml_backup"]):
            continue
        for file in files:
            if file.endswith(".zn") or file.endswith(".zen"):
                migrate_file(os.path.join(root, file))


if __name__ == "__main__":
    walk_and_migrate("/Users/yldyagz/novaRoad/nova/zn/")
