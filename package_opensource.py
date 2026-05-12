# ╔═══════════════════════════════════════════════════════════════════════════╗
# ║  Nova — OPEN-SOURCE COMMUNITY EDITION PACKAGER (package_opensource.py)     ║
# ║  Strict Separation of Free Community and Paid Enterprise Modules          ║
# ╚═══════════════════════════════════════════════════════════════════════════╝

import os
import shutil

print("🦁 Nova: Open-Source Community Edition paketleme süreci başlatılıyor...")

# Target and Source Paths
src_root = "/Users/os2026/Downloads/novaRoad 2/nova"
dest_root = "/Users/os2026/Downloads/novaRoad 2/nova/nova-community"

# Clean destination if exists
if os.path.exists(dest_root):
    shutil.rmtree(dest_root)
os.makedirs(dest_root)

# Lists of Folders and Files to EXCLUDE from the Open-Source release
# These are proprietary features (Enterprise Edition) like GPUs, advanced Unit Algebra, blockchain, premium FHE, etc.
EXCLUDED_PATHS = [
    # GPU / Hardware Acceleration
    "zn/advanced",
    "zn/realtime_matrix",
    "zn/kernel",
    "gpu-army",
    "nova_gemm",
    "zn/ffi_bridge_m1.zn",
    "zn/runtime_m1.zn",
    "zn/m1_main_demo.zn",
    
    # Advanced Unit Algebra / Scientific Simulations dependent on them
    "zn/stdlib/science/physics",
    "zn/stdlib/science/chemistry",
    "zn/stdlib/science/quantum",
    "zn/stdlib/nvidia",
    "zn/stdlib/ee",
    
    # Distributed GPU & Blockchain (Proof of Useful Work)
    "zn/stdlib/blockchain.zn",
    "zn/stdlib/distributed_computing.zn",
    
    # Premium Cryptographic & Formal Security (FHE, Constant-Time, Sandboxing)
    "zn/security/formal_security/noninterference.zn", # Homomorphic Encryption
    "zn/security/formal_security/constant_time.zn",    # Side-Channel timing protection
    "zn/security/crypto_primitives.zn",                # Secure memory zeroing & Anti-debugging
]

# Walk through source root and copy files
for root, dirs, files in os.walk(src_root):
    # Skip any hidden dirs, build dirs, or destination itself
    if "build" in root or ".git" in root or "nova-community" in root or ".vscode" in root or ".cache" in root:
        continue
        
    for file in files:
        src_file_path = os.path.join(root, file)
        rel_path = os.path.relpath(src_file_path, src_root)
        
        # Check if this file or its parent directories are in the EXCLUDE list
        should_exclude = False
        for excl in EXCLUDED_PATHS:
            if rel_path.startswith(excl) or excl in rel_path:
                should_exclude = True
                break
                
        if should_exclude:
            continue
            
        # Ensure destination folder exists
        dest_file_path = os.path.join(dest_root, rel_path)
        dest_dir = os.path.dirname(dest_file_path)
        if not os.path.exists(dest_dir):
            os.makedirs(dest_dir)
            
        # Copy the file
        shutil.copy2(src_file_path, dest_file_path)

# Generate a beautiful, professional README for the Open-Source Release
readme_content = """# 🌌 Nova (Zenith) Programming Language — Community Edition

Welcome to the **Nova Programming Language Community Edition**! Nova is a sovereign, zero-overhead, statically-typed systems programming language designed for ultra-performance, type safety, and modern software engineering.

## 🚀 Community Edition Features

- **Ultra-Compact Native WASM Compilation:** Build ultra-lightweight web applications compiling directly to WASM.
- **Sovereign UI Framework (`react.zn`):** A zero-overhead declarative UI framework with native virtual DOM.
- **Sovereign Math & Analytics:** Includes `numpy.zn` (CPU-based tensor math with automatic differentiation) and `pandas.zn` (CSV analytics parser).
- **Core Standard Library:** Advanced collections, networking, filesystem buffers, and multi-threading models.
- **Secure Package Management (`znm`):** Integrated digital signature verification and standard security audits.
- **Zenith Language Server (`zls`):** Real-time diagnostics, autocomplete, and static safety warnings for IDEs.

## 💼 Enterprise Edition (Commercial Upgrades Available)

For mission-critical applications, hardware acceleration, and financial/security infrastructure, consider upgrading to **Nova Enterprise Edition**:

1. **Hardware & GPU Acceleration (GPU Tensors):** High-performance CUDA, Vulkan, and macOS Metal GPU matrix solvers.
2. **Advanced Unit Algebra Dimensions:** Enforce physical SI/CSS units directly in compile-time layout constraints.
3. **Dağıtık Hesaplama (Distributed Computing):** Actor-model distributed nodes integrated with Proof-of-Useful-Work (PoUW) blockchain rewards.
4. **Askeri Sınıf Kriptografi & Güvenlik:** Fully Homomorphic Encryption (FHE), Secure Memory Zeroing (prevent RAM dumping), Constant-Time side-channel timers, and Anti-Debugging shields.

## 🔨 Quick Start

To build the compiler and standard library:
```bash
./BUILD_AND_RUN.sh test.zn
```

---
*Nova is designed with love by the sovereign Zenith team.*
"""

with open(os.path.join(dest_root, "README.md"), "w") as f:
    f.write(readme_content)

print("✅ Open-Source Community Edition başarıyla 'nova-community' klasörüne ayrıştırıldı!")
print("Proprietary (GPU, Advanced Unit Algebra, FHE, Blockchain) dosyalar tamamen filtrelendi.")
