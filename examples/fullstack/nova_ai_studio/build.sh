#!/usr/bin/env bash
# Nova AI Studio - Build Script
# Builds all platforms: Web, Desktop, Mobile, Server

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

echo -e "${MAGENTA}"
cat << 'EOF'
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║         🚀 NOVA AI STUDIO - BUILD SCRIPT 🚀                  ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
EOF
echo -e "${NC}"

BUILD_ALL=false
BUILD_WEB=false
BUILD_DESKTOP=false
BUILD_MOBILE=false
BUILD_SERVER=false

# Parse arguments
for arg in "$@"; do
    case $arg in
        --all)
            BUILD_ALL=true
            ;;
        --web)
            BUILD_WEB=true
            ;;
        --desktop)
            BUILD_DESKTOP=true
            ;;
        --mobile)
            BUILD_MOBILE=true
            ;;
        --server)
            BUILD_SERVER=true
            ;;
    esac
done

# If no args, build all
if [ "$BUILD_ALL" = false ] && [ "$BUILD_WEB" = false ] && [ "$BUILD_DESKTOP" = false ] && [ "$BUILD_MOBILE" = false ] && [ "$BUILD_SERVER" = false ]; then
    BUILD_ALL=true
fi

# Build Web Application
build_web() {
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}Building Web Application...${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    cd web
    
    # Compile Nova to WebAssembly
    echo -e "${BLUE}  Compiling to WebAssembly...${NC}"
    nova build App.zn --backend wasm --ssr --output ../dist/web/app.wasm
    
    # Generate HTML
    echo -e "${BLUE}  Generating HTML...${NC}"
    cat > ../dist/web/index.html << 'HTML'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Nova AI Studio</title>
    <link rel="icon" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🚀</text></svg>">
</head>
<body>
    <div id="nova-ai-studio"></div>
    <script type="module" src="/app.js"></script>
</body>
</html>
HTML
    
    # Copy assets
    echo -e "${BLUE}  Copying assets...${NC}"
    mkdir -p ../dist/web/assets
    
    cd ..
    
    echo -e "${GREEN}✅ Web build complete!${NC}"
    echo -e "${YELLOW}   Output: dist/web/${NC}"
    echo ""
}

# Build Desktop Application
build_desktop() {
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}Building Desktop Application...${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    cd desktop
    
    # Detect platform
    if [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macos"
        TARGET="darwin-arm64"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="linux"
        TARGET="linux-x86_64"
    else
        PLATFORM="windows"
        TARGET="windows-x86_64"
    fi
    
    echo -e "${BLUE}  Platform: $PLATFORM${NC}"
    echo -e "${BLUE}  Target: $TARGET${NC}"
    
    # Compile with GPU support
    echo -e "${BLUE}  Compiling with GPU acceleration...${NC}"
    if [ "$PLATFORM" = "macos" ]; then
        nova build main.zn \
            --target $TARGET \
            --gpu metal \
            --optimize 3 \
            --output ../dist/desktop/NovaAIStudio.app
    else
        nova build main.zn \
            --target $TARGET \
            --gpu vulkan \
            --optimize 3 \
            --output ../dist/desktop/nova-ai-studio
    fi
    
    cd ..
    
    echo -e "${GREEN}✅ Desktop build complete!${NC}"
    echo -e "${YELLOW}   Output: dist/desktop/${NC}"
    echo ""
}

# Build Mobile Application
build_mobile() {
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}Building Mobile Application...${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    cd mobile
    
    # Build iOS
    echo -e "${BLUE}  Building iOS app...${NC}"
    nova build App.zn \
        --target ios \
        --arch arm64 \
        --output ../dist/mobile/NovaAIStudio.app
    
    # Build Android
    echo -e "${BLUE}  Building Android app...${NC}"
    nova build App.zn \
        --target android \
        --arch arm64-v8a \
        --output ../dist/mobile/NovaAIStudio.apk
    
    cd ..
    
    echo -e "${GREEN}✅ Mobile builds complete!${NC}"
    echo -e "${YELLOW}   iOS: dist/mobile/NovaAIStudio.app${NC}"
    echo -e "${YELLOW}   Android: dist/mobile/NovaAIStudio.apk${NC}"
    echo ""
}

# Build Server
build_server() {
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}Building Server...${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    cd server
    
    echo -e "${BLUE}  Compiling server binary...${NC}"
    nova build main.zn \
        --optimize 3 \
        --output ../dist/server/nova-ai-server
    
    cd ..
    
    echo -e "${GREEN}✅ Server build complete!${NC}"
    echo -e "${YELLOW}   Output: dist/server/${NC}"
    echo ""
}

# Create dist directories
mkdir -p dist/web dist/desktop dist/mobile dist/server

# Execute builds
if [ "$BUILD_ALL" = true ] || [ "$BUILD_WEB" = true ]; then
    build_web
fi

if [ "$BUILD_ALL" = true ] || [ "$BUILD_DESKTOP" = true ]; then
    build_desktop
fi

if [ "$BUILD_ALL" = true ] || [ "$BUILD_MOBILE" = true ]; then
    build_mobile
fi

if [ "$BUILD_ALL" = true ] || [ "$BUILD_SERVER" = true ]; then
    build_server
fi

# Summary
echo -e "${MAGENTA}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${MAGENTA}║${NC}  ${GREEN}✅ BUILD COMPLETE!${NC}                                        ${MAGENTA}║${NC}"
echo -e "${MAGENTA}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}📦 Build Outputs:${NC}"
echo ""

if [ "$BUILD_ALL" = true ] || [ "$BUILD_WEB" = true ]; then
    echo -e "${BLUE}  Web:${NC}     dist/web/"
fi

if [ "$BUILD_ALL" = true ] || [ "$BUILD_DESKTOP" = true ]; then
    echo -e "${BLUE}  Desktop:${NC} dist/desktop/"
fi

if [ "$BUILD_ALL" = true ] || [ "$BUILD_MOBILE" = true ]; then
    echo -e "${BLUE}  Mobile:${NC}  dist/mobile/"
fi

if [ "$BUILD_ALL" = true ] || [ "$BUILD_SERVER" = true ]; then
    echo -e "${BLUE}  Server:${NC}  dist/server/"
fi

echo ""
echo -e "${CYAN}🚀 Quick Start:${NC}"
echo ""
echo "  # Run web app:"
echo "  cd dist/web && python3 -m http.server 8080"
echo ""
echo "  # Run desktop app:"
echo "  ./dist/desktop/nova-ai-studio"
echo ""
echo "  # Run server:"
echo "  ./dist/server/nova-ai-server"
echo ""
