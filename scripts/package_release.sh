#!/usr/bin/env bash
# Nova Language - Release Packaging Script
# Version: 1.0.0

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Detect platform
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"
VERSION="1.0.0-sovereign"
PACKAGE_NAME="nova-v${VERSION}-${OS}-${ARCH}"
RELEASE_DIR="/tmp/${PACKAGE_NAME}"

echo -e "${BLUE}📦 Preparing Nova Release: ${PACKAGE_NAME}${NC}"

# 1. Ensure a fresh build
echo -e "${BLUE}🔨 Building components...${NC}"
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)
cd ..

# 2. Setup release structure
echo -e "${BLUE}📂 Creating distribution structure...${NC}"
rm -rf "${RELEASE_DIR}"
mkdir -p "${RELEASE_DIR}/bin"
mkdir -p "${RELEASE_DIR}/binaries"
mkdir -p "${RELEASE_DIR}/include"
mkdir -p "${RELEASE_DIR}/stdlib"
mkdir -p "${RELEASE_DIR}/zn"

# 3. Copy binaries & tools
echo -e "${BLUE}📋 Copying files...${NC}"

# CLI Wrappers
cp bin/nova "${RELEASE_DIR}/bin/"
cp bin/znc "${RELEASE_DIR}/bin/"
cp bin/znrepl "${RELEASE_DIR}/bin/"
[ -f bin/nova-ai ] && cp bin/nova-ai "${RELEASE_DIR}/bin/"

# Native Binaries (from build dir)
cp build/tools/znc "${RELEASE_DIR}/binaries/"
cp build/tools/znrepl "${RELEASE_DIR}/binaries/"
[ -f build/tools/znlsp ] && cp build/tools/znlsp "${RELEASE_DIR}/binaries/"

# Standard Libraries, Headers, and Source (Recursive)
cp -pPR include "${RELEASE_DIR}/"
cp -pPR stdlib "${RELEASE_DIR}/"
cp -pPR zn "${RELEASE_DIR}/"

# Installer & License
cp install.sh "${RELEASE_DIR}/"
[ -f LICENSE ] && cp LICENSE "${RELEASE_DIR}/"
[ -f README.md ] && cp README.md "${RELEASE_DIR}/"
[ -f README_NEW.md ] && cp README_NEW.md "${RELEASE_DIR}/"

# 4. Create Tarball
echo -e "${BLUE}🗜️  Creating tarball...${NC}"
cd /tmp
tar -czf "${PACKAGE_NAME}.tar.gz" "${PACKAGE_NAME}"
cd - > /dev/null

mkdir -p binaries
cp "/tmp/${PACKAGE_NAME}.tar.gz" "binaries/"

# 5. Generate Checksum
echo -e "${BLUE}📝 Generating checksum...${NC}"
shasum -a 256 "binaries/${PACKAGE_NAME}.tar.gz" > "binaries/${PACKAGE_NAME}.tar.gz.sha256"

echo -e "${GREEN}✅ Release package prepared successfully!${NC}"
echo -e "${CYAN}   Package: binaries/${PACKAGE_NAME}.tar.gz${NC}"
echo -e "${CYAN}   Size: $(du -h "binaries/${PACKAGE_NAME}.tar.gz" | cut -f1)${NC}"
echo ""
echo -e "${YELLOW}To test the installation:${NC}"
echo -e "  tar -xzf binaries/${PACKAGE_NAME}.tar.gz"
echo -e "  cd ${PACKAGE_NAME} && ./install.sh"
