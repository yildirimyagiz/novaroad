import os
import subprocess

root = "/Users/os2026/Downloads/novaRoad/nova/src/compiler/bootstrap/mainCompiler/"


def bootstrap_all():
    for dirpath, dirnames, filenames in os.walk(root):
        if "__pycache__" in dirpath or "frontend/fixed" in dirpath:
            continue

        compiler_script = next(
            (f for f in filenames if f.endswith("_compiler.py")), None
        )
        if compiler_script:
            module_name = compiler_script.rsplit("_compiler.py", 1)[0]
            print(f"🚀 Bootstrapping module: {module_name} in {dirpath}")

            # 1. Run compiler to generate python enums
            # NEW: No nova_ prefix for implementation
            py_file = os.path.join(dirpath, f"{module_name}.py")
            try:
                subprocess.run(
                    ["python3", compiler_script, "--emit-python"],
                    cwd=dirpath,
                    stdout=open(py_file, "w"),
                    check=True,
                )
            except Exception as e:
                print(f"  ❌ Failed to generate python for {module_name}: {e}")

            # 2. Run compiler to generate C tags
            # NEW: No nova_ prefix for tags
            tag_file = os.path.join(dirpath, f"{module_name}_tags.h")
            try:
                subprocess.run(
                    ["python3", compiler_script, "--emit-c"],
                    cwd=dirpath,
                    stdout=open(tag_file, "w"),
                    check=True,
                )
            except Exception as e:
                print(f"  ❌ Failed to generate C tags for {module_name}: {e}")

            # 3. Create FFI bridge
            # NEW: No nova_ prefix for ffi
            ffi_file = os.path.join(dirpath, f"{module_name}_ffi.py")
            if not os.path.exists(ffi_file):
                with open(ffi_file, "w") as f:
                    f.write(f"import ctypes\n\n# Nova {module_name} FFI Bridge\n")
                print(f"  📝 Created FFI bridge: {ffi_file}")

            # 4. Create __init__.py
            init_file = os.path.join(dirpath, "__init__.py")
            if not os.path.exists(init_file):
                with open(init_file, "w") as f:
                    f.write(
                        f"from .{module_name} import *\nfrom .{module_name}_ffi import *\n"
                    )
                print(f"  📝 Created __init__.py")


if __name__ == "__main__":
    bootstrap_all()
