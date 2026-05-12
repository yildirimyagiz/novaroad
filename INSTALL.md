# Nova Compiler Installation Guide

# Version: 1.0.0-rc1

## Overview

Nova is a systems programming language with a focus on safety, performance, and ease of use.
This guide covers installation of the Nova compiler and toolchain for v1.0.0-rc1.

## System Requirements

### Minimum Requirements

- **Operating System**: macOS 13.0+, Linux (Ubuntu 20.04+), Windows 10+
- **Architecture**: x86_64 (64-bit)
- **RAM**: 2GB minimum, 4GB recommended
- **Disk Space**: 100MB for installation
- **Dependencies**:
  - LLVM 16.0+ (for compilation)
  - GCC/Clang (bootstrap compiler)

### Recommended Requirements

- **RAM**: 8GB or more
- **CPU**: Multi-core processor (4+ cores)
- **Disk Space**: 500MB for development

## Installation Methods

### Method 1: Pre-compiled Binaries (Recommended)

#### macOS (x86_64)

```bash
# Download the release archive
curl -L -o nova-v1.0.0-rc1-macos-x86_64.tar.gz \
  https://github.com/nova/nova/releases/download/v1.0.0-rc1/nova-v1.0.0-rc1-macos-x86_64.tar.gz

# Verify checksum
shasum -a 256 nova-v1.0.0-rc1-macos-x86_64.tar.gz

# Extract
tar -xzf nova-v1.0.0-rc1-macos-x86_64.tar.gz
cd nova-v1.0.0-rc1

# Add to PATH (optional)
export PATH="$PWD/bin:$PATH"
```

#### Linux (x86_64)

```bash
# Download the release archive
wget https://github.com/nova/nova/releases/download/v1.0.0-rc1/nova-v1.0.0-rc1-linux-x86_64.tar.gz

# Verify checksum
sha256sum nova-v1.0.0-rc1-linux-x86_64.tar.gz

# Extract and install
tar -xzf nova-v1.0.0-rc1-linux-x86_64.tar.gz
cd nova-v1.0.0-rc1
sudo cp bin/nova /usr/local/bin/
sudo cp -r lib/nova /usr/local/lib/
```

#### Windows (x86_64)

```powershell
# Download using PowerShell
Invoke-WebRequest -Uri "https://github.com/nova/nova/releases/download/v1.0.0-rc1/nova-v1.0.0-rc1-windows-x86_64.zip" -OutFile "nova-v1.0.0-rc1-windows-x86_64.zip"

# Extract using 7-Zip or built-in Windows extraction
# Add bin/ to PATH environment variable
```

### Method 2: Build from Source

#### Prerequisites

```bash
# macOS
brew install llvm@16 cmake ninja

# Ubuntu/Debian
sudo apt update
sudo apt install llvm-16-dev clang-16 cmake ninja-build

# CentOS/RHEL
sudo yum install llvm16-devel clang16 cmake ninja-build
```

#### Build Steps

```bash
# Clone the repository
git clone https://github.com/nova/nova.git
cd nova

# Checkout the release tag
git checkout v1.0.0-rc1

# Configure build
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/usr/lib/llvm-16/cmake

# Build
ninja -C build

# Install (optional)
sudo ninja -C build install
```

## Verification

After installation, verify Nova is working:

```bash
# Check version
nova --version
# Should output: Nova Compiler v1.0.0-rc1

# Compile a simple program
echo 'fn main() { println("Hello, Nova!"); }' > hello.nv
nova build hello.nv
./hello
# Should output: Hello, Nova!
```

## IDE Integration

### Visual Studio Code

1. Install the "Nova Language Support" extension
2. Configure the Nova binary path in settings:

   ```json
   {
     "nova.compiler.path": "/usr/local/bin/nova"
   }
   ```

### Vim/Neovim

Add to your `.vimrc`:

```vim
autocmd BufNewFile,BufRead *.nv set filetype=nova
```

### Emacs

Add to your `init.el`:

```elisp
(add-to-list 'auto-mode-alist '("\\.nv\\'" . nova-mode))
```

## Package Management

Nova uses a package manager for dependency management:

```bash
# Initialize a new project
nova init my-project
cd my-project

# Edit nova.toml for dependencies
# Build the project
nova build

# Run tests
nova test
```

## Troubleshooting

### Common Issues

#### "LLVM not found" during build

- Ensure LLVM 16+ is installed
- Set `LLVM_DIR` environment variable to LLVM cmake directory

#### "Permission denied" during install

- Use `sudo` for system-wide installation
- Or install to user directory: `cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local`

#### Bootstrap failures

- Ensure you have a working C compiler (GCC/Clang)
- Check that all dependencies are installed
- Try building in debug mode for more verbose output

### Getting Help

- **Documentation**: <https://nova.org/docs>
- **Issues**: <https://github.com/nova/nova/issues>
- **Discussions**: <https://github.com/nova/nova/discussions>
- **Discord**: <https://discord.gg/nova>

## Uninstallation

### Pre-compiled Installation

```bash
# Remove the installation directory
rm -rf /path/to/nova-v1.0.0-rc1

# Remove from PATH if added
# Edit your shell profile (.bashrc, .zshrc, etc.)
```

### System Installation

```bash
# macOS/Linux
sudo rm /usr/local/bin/nova
sudo rm -rf /usr/local/lib/nova

# Windows
# Remove from PATH, delete installation directory
```

---

For more information, visit the [Nova website](https://nova.org) or check the [documentation](https://nova.org/docs).
