#!/usr/bin/env bash
# Nova Language - Installation Script
# Version: 9.0.0

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

NOVA_VERSION="9.0.0-sovereign"

# Show banner
echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║${NC}          ${MAGENTA}Nova Language - Installation Script${NC}              ${CYAN}║${NC}"
echo -e "${CYAN}║${NC}          ${BLUE}Version ${NOVA_VERSION}${NC}                        ${CYAN}║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check if Nova is already installed
if command -v nova &> /dev/null; then
    echo -e "${GREEN}✅ Nova is already installed!${NC}"
    nova --version
    echo ""
    read -p "Do you want to reinstall? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 0
    fi
fi

echo -e "${BLUE}📦 Installing Nova Language...${NC}"
echo ""

# Detect OS and Architecture
OS="$(uname -s)"
ARCH="$(uname -m)"

case "${OS}" in
    Linux*)     PLATFORM="linux";;
    Darwin*)    PLATFORM="macos";;
    CYGWIN*)    PLATFORM="windows";;
    MINGW*)     PLATFORM="windows";;
    *)          PLATFORM="unknown";;
esac

case "${ARCH}" in
    x86_64)     ARCH="x86_64";;
    arm64)      ARCH="arm64";;
    aarch64)    ARCH="arm64";;
    *)          ARCH="unknown";;
esac

echo -e "${CYAN}   Platform detected: ${PLATFORM}-${ARCH}${NC}"

if [ "$PLATFORM" = "unknown" ] || [ "$ARCH" = "unknown" ]; then
    echo -e "${RED}❌ Error: Unsupported platform: ${OS} ${ARCH}${NC}"
    exit 1
fi

# Installation directory
INSTALL_TYPE="local"
NOVA_HOME="$HOME/.nova"

echo ""
echo "Installation options:"
echo "  1) Local installation (recommended) - installs to ~/.nova"
echo "  2) System-wide installation - installs to /usr/local (requires sudo)"
echo ""
read -p "Choose installation type (1 or 2, default: 1): " -n 1 -r
echo ""

if [[ $REPLY =~ ^2$ ]]; then
    INSTALL_TYPE="system"
    NOVA_HOME="/usr/local/lib/nova"
    BIN_DIR="/usr/local/bin"
else
    INSTALL_TYPE="local"
    NOVA_HOME="$HOME/.nova"
    BIN_DIR="$NOVA_HOME/bin"
fi

echo -e "${CYAN}   Installing to: ${NOVA_HOME}${NC}"
echo ""

# Create Nova directory
if [ "$INSTALL_TYPE" = "system" ]; then
    sudo mkdir -p "${NOVA_HOME}"
    sudo mkdir -p "${BIN_DIR}"
else
    mkdir -p "${NOVA_HOME}"
    mkdir -p "${BIN_DIR}"
fi

# Copy files
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Stage fresh binaries
mkdir -p "${SCRIPT_DIR}/binaries"
cp "${SCRIPT_DIR}/build/tools/znc" "${SCRIPT_DIR}/binaries/" 2>/dev/null || true
cp "${SCRIPT_DIR}/build/tools/znrepl" "${SCRIPT_DIR}/binaries/" 2>/dev/null || true

echo -e "${BLUE}📋 Copying files...${NC}"

if [ "$INSTALL_TYPE" = "system" ]; then
    sudo cp -r "${SCRIPT_DIR}/bin/"* "${BIN_DIR}/"
    sudo cp -r "${SCRIPT_DIR}/binaries" "${NOVA_HOME}/"
    sudo cp -r "${SCRIPT_DIR}/zn" "${NOVA_HOME}/" 2>/dev/null || true
    sudo cp -r "${SCRIPT_DIR}/stdlib" "${NOVA_HOME}/" 2>/dev/null || true
    
    sudo chmod +x "${BIN_DIR}/nova"
    sudo chmod +x "${BIN_DIR}/znc"
    sudo chmod +x "${BIN_DIR}/znrepl"
    sudo chmod +x "${BIN_DIR}/nova-ai"
else
    cp -r "${SCRIPT_DIR}/bin/"* "${BIN_DIR}/"
    cp -r "${SCRIPT_DIR}/binaries" "${NOVA_HOME}/"
    cp -r "${SCRIPT_DIR}/zn" "${NOVA_HOME}/" 2>/dev/null || true
    cp -r "${SCRIPT_DIR}/stdlib" "${NOVA_HOME}/" 2>/dev/null || true
    
    chmod +x "${BIN_DIR}/nova"
    chmod +x "${BIN_DIR}/znc"
    chmod +x "${BIN_DIR}/znrepl"
    chmod +x "${BIN_DIR}/nova-ai"
fi

echo -e "${GREEN}   ✓ Files copied${NC}"

# Add to PATH
if [ "$INSTALL_TYPE" = "local" ]; then
    echo ""
    echo -e "${BLUE}🔧 Setting up environment...${NC}"
    
    # Detect shell
    SHELL_RC=""
    if [ -n "$BASH_VERSION" ]; then
        SHELL_RC="$HOME/.bashrc"
    elif [ -n "$ZSH_VERSION" ]; then
        SHELL_RC="$HOME/.zshrc"
    elif [ -f "$HOME/.zshrc" ]; then
        SHELL_RC="$HOME/.zshrc"
    elif [ -f "$HOME/.bashrc" ]; then
        SHELL_RC="$HOME/.bashrc"
    elif [ -f "$HOME/.profile" ]; then
        SHELL_RC="$HOME/.profile"
    fi
    
    if [ -n "$SHELL_RC" ]; then
        # Check if already in PATH
        if ! grep -q "NOVA_HOME" "$SHELL_RC" 2>/dev/null; then
            echo "" >> "$SHELL_RC"
            echo "# Nova Language" >> "$SHELL_RC"
            echo "export NOVA_HOME=\"${NOVA_HOME}\"" >> "$SHELL_RC"
            echo "export PATH=\"\$PATH:${BIN_DIR}\"" >> "$SHELL_RC"
            echo -e "${GREEN}   ✓ Added to ${SHELL_RC}${NC}"
        else
            echo -e "${YELLOW}   ⚠️  Already in PATH${NC}"
        fi
    fi
    
    # Export for current session
    export NOVA_HOME="${NOVA_HOME}"
    export PATH="$PATH:${BIN_DIR}"
fi

echo ""
echo -e "${GREEN}✅ Nova installation complete!${NC}"
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${YELLOW}To get started:${NC}"

if [ "$INSTALL_TYPE" = "local" ]; then
    echo -e "  1. ${CYAN}Restart your shell or run:${NC} source ${SHELL_RC}"
    echo -e "  2. ${CYAN}Create your first project:${NC} nova new my-app"
    echo -e "  3. ${CYAN}Run it:${NC} cd my-app && nova build && nova run"
else
    echo -e "  1. ${CYAN}Create your first project:${NC} nova new my-app"
    echo -e "  2. ${CYAN}Run it:${NC} cd my-app && nova build && nova run"
fi

echo ""
echo -e "${YELLOW}Quick commands:${NC}"
echo "  nova new <name>     Create a new project"
echo "  nova build          Build your project"
echo "  nova run            Run your project"
echo "  nova repl           Start interactive REPL"
echo "  nova --help         Show all commands"
echo ""
echo -e "${CYAN}📚 Documentation:${NC} https://nova.org/docs"
echo -e "${CYAN}💬 Community:${NC} https://discord.gg/nova"
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Test installation
if [ "$INSTALL_TYPE" = "system" ] || echo "$PATH" | grep -q "$BIN_DIR"; then
    echo -e "${BLUE}🧪 Testing installation...${NC}"
    if command -v nova &> /dev/null; then
        nova --version
        echo -e "${GREEN}✅ Installation verified!${NC}"
    else
        echo -e "${YELLOW}⚠️  Nova command not found in PATH yet${NC}"
        echo -e "${YELLOW}   Please restart your shell or run: source ${SHELL_RC}${NC}"
    fi
fi

echo ""
echo -e "${GREEN}🎉 Happy coding with Nova!${NC}"
