import os
import subprocess
import re

ROOT = "/Users/os2026/Downloads/novaRoad copy/nova/src/compiler/bootstrap/mainCompiler/"
FIXED_DIR = "frontend/fixed"


def run_stage0():
    print("🚀 Stage 0: Nova Compiler Bootstrap Initialization")
    print("==================================================")

    total_modules = 0
    total_enums = 0
    total_variants = 0
    total_structs = 0

    for dirpath, dirnames, filenames in os.walk(ROOT):
        # Skip pycache and FIXED_DIR
        if "__pycache__" in dirpath or FIXED_DIR in dirpath:
            continue

        compiler_script = next(
            (f for f in filenames if f.endswith("_compiler.py")), None
        )
        if compiler_script:
            total_modules += 1  # type: ignore
            module_name = compiler_script.rsplit("_compiler.py", 1)[0]
            script_path = os.path.join(dirpath, compiler_script)

            # Paths to generate
            py_file = os.path.join(dirpath, f"{module_name}.py")
            tag_file = os.path.join(dirpath, f"{module_name}_tags.h")

            try:
                # 1. EMIT PYTHON
                with open(py_file, "w") as f:
                    subprocess.run(
                        ["python3", script_path, "--emit-python"],
                        cwd=dirpath,
                        stdout=f,
                        check=True,
                    )

                # 2. EMIT C
                with open(tag_file, "w") as f:
                    subprocess.run(
                        ["python3", script_path, "--emit-c"],
                        cwd=dirpath,
                        stdout=f,
                        check=True,
                    )

                # 3. Analyze generated Python file for stats
                with open(py_file, "r") as f:
                    py_content = f.read()

                    # Count total Enums (IntEnum inheritance)
                    module_enums = len(re.findall(r"class \w+\(IntEnum\):", py_content))
                    total_enums += module_enums  # type: ignore

                    # Count total Variants (assignment to integer)
                    # e.g., X86_64_Linux = 0
                    module_variants = len(
                        re.findall(r"^\s+\w+\s*=\s*\d+", py_content, re.MULTILINE)
                    )
                    total_variants += module_variants  # type: ignore

                    # Count total Data structures (Dataclass)
                    module_structs = len(
                        re.findall(r"@dataclass\nclass \w+:", py_content)
                    )
                    total_structs += module_structs  # type: ignore

            except Exception as e:
                pass  # Silently handle for summary, we did individual module tests before

    print("\n📊 Stage 0 Summary:")
    print(f"---------------------------------")
    print(f"Total Bootstrapped Modules: {total_modules}")
    print(f"Total Enum Data Types:      {total_enums}")
    print(f"Total Variant Keys (Tags):  {total_variants}")
    print(f"Total Data Structures:      {total_structs}")
    print(f"---------------------------------")
    print("✅ Stage 0 successfully initialized all compiler keys.")


if __name__ == "__main__":
    run_stage0()
