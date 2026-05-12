# Nova Build Guide

## Prerequisites

### Required Tools

- C11 compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.20+ (recommended) or Make
- Python 3.8+ (for build scripts)

### Optional Tools

- LLVM 14+ (for LLVM backend)
- Meson + Ninja (alternative build system)
- clang-format, clang-tidy (for code quality)
- Doxygen (for documentation)

## Quick Start

### CMake Build (Recommended)

```bash
# Clone repository
cd nova

# Configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run tests
ctest

# Install (optional)
sudo make install
```

### Make Build

```bash
cd nova
make -j$(nproc)
make tests
sudo make install
```

### Meson Build

```bash
cd nova
meson setup build --buildtype=release
cd build
ninja
ninja test
sudo ninja install
```

## Build Options

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `NOVA_BUILD_KERNEL` | ON | Build kernel subsystem |
| `NOVA_BUILD_RUNTIME` | ON | Build runtime system |
| `NOVA_BUILD_COMPILER` | ON | Build compiler |
| `NOVA_BUILD_AI` | ON | Build AI subsystem |
| `NOVA_BUILD_TOOLS` | ON | Build developer tools |
| `NOVA_BUILD_TESTS` | ON | Build test suite |
| `NOVA_ENABLE_LTO` | OFF | Enable Link-Time Optimization |
| `NOVA_ENABLE_SANITIZERS` | OFF | Enable ASan/UBSan |

### Example: Minimal Build

```bash
cmake .. \
  -DNOVA_BUILD_KERNEL=OFF \
  -DNOVA_BUILD_AI=OFF \
  -DNOVA_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=MinSizeRel
```

### Example: Development Build

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DNOVA_ENABLE_SANITIZERS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## Architecture-Specific Builds

### Cross-Compilation for ARM64

```bash
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
```

### WebAssembly Build

```bash
emcmake cmake .. \
  -DNOVA_BUILD_KERNEL=OFF \
  -DCMAKE_BUILD_TYPE=Release
make
```

## Build Artifacts

After building, you'll find:

```
build/
├── bin/              # Executables
│   ├── znc          # Compiler
│   └── znrepl       # REPL
├── lib/              # Static libraries
│   ├── libnova_kernel.a
│   ├── libnova_runtime.a
│   ├── libnova_compiler.a
│   ├── libnova_ai.a
│   ├── libnova_security.a
│   └── libnova_std.a
└── compile_commands.json  # For LSP/IDE
```

## Testing

### Run All Tests

```bash
cd build
ctest --verbose
```

### Run Specific Test Suite

```bash
# Unit tests
./test_runner

# Integration tests
./integration_tests

# Benchmarks
./benchmarks/bench_compiler
```

## Code Quality

### Format Code

```bash
clang-format -i src/**/*.c include/**/*.h
```

### Run Static Analysis

```bash
clang-tidy src/**/*.c -- -Iinclude
```

### Generate Documentation

```bash
doxygen Doxyfile
```

## Troubleshooting

### Common Issues

**Issue**: `compile_commands.json` not generated

```bash
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

**Issue**: Missing LLVM

```bash
# Disable LLVM backend
cmake .. -DNOVA_USE_LLVM=OFF
```

**Issue**: Out of memory during build

```bash
# Reduce parallelism
make -j2
```

### Platform-Specific Notes

#### macOS

- Install Xcode Command Line Tools: `xcode-select --install`
- Use Homebrew for dependencies: `brew install cmake llvm`

#### Linux

- Install build essentials: `sudo apt install build-essential cmake`
- For LLVM: `sudo apt install llvm-14-dev`

#### Windows

- Use Visual Studio 2019+ with CMake support
- Or use WSL2 for Unix-like environment

## Performance Optimization

### Link-Time Optimization

```bash
cmake .. -DNOVA_ENABLE_LTO=ON -DCMAKE_BUILD_TYPE=Release
```

### Profile-Guided Optimization

```bash
# 1. Build with instrumentation
cmake .. -DCMAKE_C_FLAGS="-fprofile-generate"
make

# 2. Run representative workload
./bin/znc examples/*.nova

# 3. Rebuild with profile data
cmake .. -DCMAKE_C_FLAGS="-fprofile-use"
make
```

## Development Workflow

### Recommended Setup

1. **IDE**: VSCode with C/C++ extension
2. **LSP**: Use `compile_commands.json`
3. **Debugger**: GDB or LLDB
4. **Build**: CMake with Ninja generator

### Example VSCode Configuration

```json
{
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json",
  "C_Cpp.default.cStandard": "c11",
  "C_Cpp.default.cppStandard": "c++17"
}
```

## Continuous Integration

The project includes CI configurations for:

- GitHub Actions
- GitLab CI
- Jenkins

See `.github/workflows/` for examples.
