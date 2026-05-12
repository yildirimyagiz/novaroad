#!/usr/bin/env bash
# Automatically fix PATH to use new Nova CLI

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

echo -e "${MAGENTA}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${MAGENTA}║${NC}          ${CYAN}Nova CLI - PATH Auto-Fix${NC}                        ${MAGENTA}║${NC}"
echo -e "${MAGENTA}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Detect shell
SHELL_RC=""
if [ -n "$ZSH_VERSION" ]; then
    SHELL_RC="$HOME/.zshrc"
    SHELL_NAME="zsh"
elif [ -n "$BASH_VERSION" ]; then
    SHELL_RC="$HOME/.bashrc"
    SHELL_NAME="bash"
elif [ -f "$HOME/.zshrc" ]; then
    SHELL_RC="$HOME/.zshrc"
    SHELL_NAME="zsh"
elif [ -f "$HOME/.bashrc" ]; then
    SHELL_RC="$HOME/.bashrc"
    SHELL_NAME="bash"
fi

echo -e "${CYAN}Detected shell:${NC} $SHELL_NAME"
echo -e "${CYAN}Config file:${NC} $SHELL_RC"
echo ""

# Check current nova
echo -e "${YELLOW}Current nova:${NC}"
which nova
nova --version 2>&1 | head -3 | grep -v "^$" || true
echo ""

# Check new nova
echo -e "${YELLOW}New nova:${NC}"
ls -lh ~/.nova/bin/nova
~/.nova/bin/nova version
echo ""

# Ask confirmation
echo -e "${YELLOW}This will update PATH to prioritize new Nova CLI.${NC}"
echo ""
read -p "Continue? (y/N) " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${BLUE}Cancelled.${NC}"
    exit 0
fi

# Backup shell config
echo -e "${CYAN}Backing up $SHELL_RC...${NC}"
cp "$SHELL_RC" "$SHELL_RC.backup.$(date +%s)"
echo -e "${GREEN}✓ Backup created${NC}"
echo ""

# Remove old Nova PATH entries (if any)
echo -e "${CYAN}Cleaning old Nova PATH entries...${NC}"
sed -i.tmp '/# Nova CLI - Priority PATH/d' "$SHELL_RC" 2>/dev/null || true
sed -i.tmp '/export PATH="$HOME\/.nova\/bin:$PATH"/d' "$SHELL_RC" 2>/dev/null || true
sed -i.tmp '/export PATH="\$PATH:\$HOME\/.nova\/bin"/d' "$SHELL_RC" 2>/dev/null || true
sed -i.tmp '/# Nova Language/,/export PATH/d' "$SHELL_RC" 2>/dev/null || true
rm -f "$SHELL_RC.tmp"
echo -e "${GREEN}✓ Cleaned${NC}"
echo ""

# Add new PATH entry at the beginning (priority)
echo -e "${CYAN}Adding new Nova CLI to PATH (priority)...${NC}"
{
    echo ""
    echo "# Nova CLI - Priority PATH (added $(date))"
    echo 'export PATH="$HOME/.nova/bin:$PATH"'
} >> "$SHELL_RC"
echo -e "${GREEN}✓ Added to $SHELL_RC${NC}"
echo ""

# Show what was added
echo -e "${YELLOW}Added to $SHELL_RC:${NC}"
tail -n 3 "$SHELL_RC"
echo ""

# Success
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║${NC}  ${MAGENTA}✅ PATH Updated Successfully!${NC}                           ${GREEN}║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

echo -e "${YELLOW}🎯 Next Steps:${NC}"
echo ""
echo -e "${CYAN}1. Reload your shell:${NC}"
echo "   source $SHELL_RC"
echo ""
echo -e "${CYAN}2. Or restart your terminal${NC}"
echo ""
echo -e "${CYAN}3. Verify new Nova:${NC}"
echo "   nova version"
echo "   # Should show: Nova v9.0.0-sovereign"
echo ""
echo -e "${CYAN}4. Test it:${NC}"
echo "   nova new my-app"
echo "   cd my-app"
echo "   # Should work now!"
echo ""

echo -e "${BLUE}📝 Note: Backup saved at:${NC}"
echo "   $SHELL_RC.backup.*"
echo ""
