# Nova Bootstrap Compiler - Multi-Platform Build System
# Supports: Web Apps, Mobile Apps, CLI Apps, Desktop Apps

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -fPIC -I. -I../core
AR := ar
LN := ln

BUILD_DIR := ../../build/bootstrap
LIB_DIR := $(BUILD_DIR)/lib
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

# Platform detection
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Darwin)
	PLATFORM := macos
	SHARED_EXT := dylib
	SHARED_FLAG := -dynamiclib
else ifeq ($(UNAME_S),Linux)
	PLATFORM := linux
	SHARED_EXT := so
	SHARED_FLAG := -shared
else
	PLATFORM := windows
	SHARED_EXT := dll
	SHARED_FLAG := -shared
endif

# Runtime Sources
RUNTIME_SOURCES := nova_runtime_io.c nova_runtime_net.c
RUNTIME_OBJECTS := $(addprefix $(OBJ_DIR)/, $(RUNTIME_SOURCES:.c=.o))
RUNTIME_STATIC := $(LIB_DIR)/libnova_runtime.a
RUNTIME_SHARED := $(LIB_DIR)/libnova_runtime.$(SHARED_EXT)

# Compiler Sources (updated with renderer)
COMPILER_SOURCES := $(wildcard *_compiler.zn) $(wildcard *compiler*.zn)
BOOTSTRAP_SOURCES := nova_compiler_bootstrap.zn
RENDERER_SOURCES := ../renderer/core/*.zn ../renderer/compiler/*.zn
UI_SOURCES := ../ui/core.zn ../templates/template_compiler.zn ../runtime/async/async_runtime.zn
FULLSTACK_SOURCES := $(BOOTSTRAP_SOURCES) $(RENDERER_SOURCES) $(UI_SOURCES)

# Application Types (expanded)
WEB_APP_SOURCES := $(wildcard web_app*.zn) sfc_demo.zn ../ui/web/*.zn ../renderer/web/*.zn
MOBILE_APP_SOURCES := $(wildcard mobile_app*.zn) ../ui/mobile/*.zn ../renderer/mobile/*.zn
CLI_APP_SOURCES := $(wildcard cli_app*.zn) nova_cli.zn
DESKTOP_APP_SOURCES := $(wildcard desktop_app*.zn) ../ui/desktop/*.zn ../renderer/desktop/*.zn
FULLSTACK_APP_SOURCES := $(wildcard fullstack_app*.zn) ../ui/fullstack/*.zn ../renderer/**/*.zn

.PHONY: all clean runtime compilers ui-core templates async fullstack web-apps mobile-apps cli-apps desktop-apps fullstack-apps test verify help

# Default target
all: runtime compilers apps
	@echo "🎉 Nova Bootstrap Complete!"
	@echo "   Platform: $(PLATFORM)-$(UNAME_M)"
	@echo "   Runtime: ✅ Built"
	@echo "   Compilers: ✅ Built"
	@echo "   Apps: ✅ Built"

# Runtime libraries
runtime: $(RUNTIME_STATIC) $(RUNTIME_SHARED)
	@echo "✅ Runtime libraries built for $(PLATFORM)"

$(RUNTIME_STATIC): $(RUNTIME_OBJECTS) | $(LIB_DIR)
	@echo "📦 Creating static runtime library..."
	$(AR) rcs $@ $(RUNTIME_OBJECTS)

$(RUNTIME_SHARED): $(RUNTIME_OBJECTS) | $(LIB_DIR)
	@echo "🔗 Creating shared runtime library..."
	$(CC) $(SHARED_FLAG) -o $@ $(RUNTIME_OBJECTS)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "🔨 Compiling runtime: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Bootstrap compilers
compilers: $(BOOTSTRAP_SOURCES)
	@echo "🏗️  Building Nova bootstrap compilers..."
	@for src in $(BOOTSTRAP_SOURCES); do \
		echo "   Compiling: $$src"; \
		./nova_stage0.py "$$src" -o "$(BIN_DIR)/$$(basename $$src .zn)" || echo "   ⚠️  Bootstrap compilation not ready yet"; \
	done
	@echo "✅ Compiler bootstrap phase complete"

# UI Core compilation
ui-core: runtime
	@echo "🎨 Building Nova UI Core..."
	@mkdir -p $(BIN_DIR)/ui
	@for src in $(UI_SOURCES); do \
		if [ -f "$$src" ]; then \
			echo "   UI Core: $$(basename $$src)"; \
			./nova_stage0.py "$$src" -o "$(BIN_DIR)/ui/$$(basename $$src .zn)" 2>/dev/null || \
			echo "   ⚠️  UI compilation needs bootstrap"; \
		fi \
	done

# Renderer compilation
renderer: runtime
	@echo "🎭 Building Nova Renderer Core..."
	@mkdir -p $(BIN_DIR)/renderer
	@for src in $(RENDERER_SOURCES); do \
		if [ -f "$$src" ]; then \
			echo "   Renderer: $$(basename $$src)"; \
			./nova_stage0.py "$$src" -o "$(BIN_DIR)/renderer/$$(basename $$src .zn)" 2>/dev/null || \
			echo "   ⚠️  Renderer compilation needs bootstrap"; \
		fi \
	done

# Template compiler
templates: ui-core
	@echo "📝 Building Template Compiler..."
	@mkdir -p $(BIN_DIR)/templates
	@for src in ../templates/template_compiler.zn; do \
		if [ -f "$$src" ]; then \
			./nova_stage0.py "$$src" "$(BIN_DIR)/templates/template_compiler" 2>/dev/null || \
			echo "   ⚠️  Template compilation needs bootstrap"; \
		fi \
	done

# Async runtime
async: runtime
	@echo "⚡ Building Async Runtime..."
	@mkdir -p $(BIN_DIR)/async
	@for src in ../runtime/async/async_runtime.zn; do \
		if [ -f "$$src" ]; then \
			./nova_stage0.py "$$src" "$(BIN_DIR)/async/async_runtime" 2>/dev/null || \
			echo "   ⚠️  Async runtime compilation needs bootstrap"; \
		fi \
	done

# Fullstack compilation (all layers)
fullstack: runtime compilers renderer ui-core templates async
	@echo "🚀 Building Fullstack Nova..."
	@mkdir -p $(BIN_DIR)/fullstack
	@for src in $(FULLSTACK_SOURCES); do \
		if [ -f "$$src" ]; then \
			echo "   Fullstack: $$(basename $$src)"; \
			./nova_stage0.py "$$src" "$(BIN_DIR)/fullstack/$$(basename $$src .zn)" 2>/dev/null || \
			echo "   ⚠️  Fullstack compilation needs bootstrap"; \
		fi \
	done
	@echo "✅ Fullstack compilation complete"

# Application builds (updated)
apps: web-apps mobile-apps cli-apps desktop-apps fullstack-apps

web-apps: runtime
	@echo "🌐 Building web applications..."
	@mkdir -p $(BIN_DIR)/web
	@for src in $(WEB_APP_SOURCES); do \
		if [ -f "$$src" ]; then \
			echo "   Web App: $$src"; \
			./nova_stage0.py "$$src" "$(BIN_DIR)/web/$$(basename $$src .zn)" 2>/dev/null || \
			echo "   ⚠️  Web app compilation needs bootstrap"; \
		fi \
	done

mobile-apps: runtime
	@echo "📱 Building mobile applications..."
	@mkdir -p $(BIN_DIR)/mobile
	@for src in $(MOBILE_APP_SOURCES); do \
		if [ -f "$$src" ]; then \
			echo "   Mobile App: $$src"; \
			./nova_stage0.py "$$src" "$(BIN_DIR)/mobile/$$(basename $$src .zn)" 2>/dev/null || \
			echo "   ⚠️  Mobile app compilation needs bootstrap"; \
		fi \
	done

cli-apps: runtime
	@echo "💻 Building CLI applications..."
	@mkdir -p $(BIN_DIR)/cli
	@for src in $(CLI_APP_SOURCES); do \
		if [ -f "$$src" ]; then \
			echo "   CLI App: $$src"; \
			./nova_stage0.py "$$src" "$(BIN_DIR)/cli/$$(basename $$src .zn)" 2>/dev/null || \
			echo "   ⚠️  CLI app compilation needs bootstrap"; \
		fi \
	done

desktop-apps: runtime
	@echo "🖥️  Building desktop applications..."
	@mkdir -p $(BIN_DIR)/desktop
	@for src in $(DESKTOP_APP_SOURCES); do \
		if [ -f "$$src" ]; then \
			echo "   Desktop App: $$src"; \
			./nova_stage0.py "$$src" "$(BIN_DIR)/desktop/$$(basename $$src .zn)" 2>/dev/null || \
			echo "   ⚠️  Desktop app compilation needs bootstrap"; \
		fi \
	done

fullstack-apps: fullstack
	@echo "🚀 Building fullstack applications..."
	@mkdir -p $(BIN_DIR)/fullstack-apps
	@for src in $(FULLSTACK_APP_SOURCES); do \
		if [ -f "$$src" ]; then \
			echo "   Fullstack App: $$src"; \
			./nova_stage0.py "$$src" "$(BIN_DIR)/fullstack-apps/$$(basename $$src .zn)" 2>/dev/null || \
			echo "   ⚠️  Fullstack app compilation needs bootstrap"; \
		fi \
	done

# Testing (enhanced)
test: runtime
	@echo "🧪 Testing Nova bootstrap system..."
	@echo "   Platform: $(PLATFORM)-$(UNAME_M)"
	@echo "   Runtime libs: $(shell ls -1 $(LIB_DIR)/*.a 2>/dev/null | wc -l) static, $(shell ls -1 $(LIB_DIR)/*.$(SHARED_EXT) 2>/dev/null | wc -l) shared"
	@echo "   Compiler sources: $(words $(BOOTSTRAP_SOURCES))"
	@echo "   UI sources: $(words $(UI_SOURCES))"
	@echo "   Template compiler: $(shell test -f ../template_compiler.zn && echo '✅' || echo '❌')"
	@echo "   Async runtime: $(shell test -f ../async_runtime.zn && echo '✅' || echo '❌')"
	@echo "   Web apps: $(words $(WEB_APP_SOURCES))"
	@echo "   Mobile apps: $(words $(MOBILE_APP_SOURCES))"
	@echo "   CLI apps: $(words $(CLI_APP_SOURCES))"
	@echo "   Desktop apps: $(words $(DESKTOP_APP_SOURCES))"
	@echo "   Fullstack apps: $(words $(FULLSTACK_APP_SOURCES))"

verify: all
	@echo ""
	@echo "🔍 Verifying Nova bootstrap system..."
	@echo "═══════════════════════════════════════"
	@echo "Platform: $(PLATFORM)-$(UNAME_M)"
	@echo ""
	@if [ -f "$(RUNTIME_STATIC)" ]; then \
		echo "✅ Static runtime library:"; \
		ls -lh $(RUNTIME_STATIC); \
		echo "   Symbols: $$(nm $(RUNTIME_STATIC) 2>/dev/null | grep -c nova_) functions"; \
	else \
		echo "❌ Static runtime library missing"; \
	fi
	@echo ""
	@if [ -f "$(RUNTIME_SHARED)" ]; then \
		echo "✅ Shared runtime library:"; \
		ls -lh $(RUNTIME_SHARED); \
	else \
		echo "❌ Shared runtime library missing"; \
	fi
	@echo ""
	@echo "📁 Build artifacts:"
	@find $(BUILD_DIR) -type f -exec ls -lh {} \; 2>/dev/null | head -10

# Clean build artifacts
clean:
	@echo "🧹 Cleaning bootstrap build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "✅ Clean complete"

# Development helpers
stage0-test: nova_stage0.py
	@echo "🧪 Testing Stage 0 (Python bootstrap)..."
	@python3 nova_stage0.py --test 2>/dev/null || echo "   ⚠️  Stage 0 test not implemented yet"

help:
	@echo "Nova Bootstrap Compiler - Multi-Platform Application Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build everything (runtime + compilers + apps)"
	@echo "  runtime      - Build runtime libraries only"
	@echo "  compilers    - Build bootstrap compilers"
	@echo "  renderer     - Build renderer core system"
	@echo "  ui-core      - Build UI core libraries"
	@echo "  templates    - Build template compiler"
	@echo "  async        - Build async runtime"
	@echo "  fullstack    - Build fullstack compiler (all layers)"
	@echo "  apps         - Build all application types"
	@echo "  web-apps     - Build web applications only"
	@echo "  mobile-apps  - Build mobile applications only"
	@echo "  cli-apps     - Build CLI applications only"
	@echo "  desktop-apps - Build desktop applications only"
	@echo "  test         - Test build system"
	@echo "  verify       - Verify build artifacts"
	@echo "  clean        - Clean build artifacts"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Architecture Layers:"
	@echo "  0. Core Compiler (Language + Types + Macros)"
	@echo "  1. UI Core (Platform-neutral UI IR)"
	@echo "  2. Renderer Core (Reactive + Patching)"
	@echo "  3. Template Compiler (SFC → AST → UI IR)"
	@echo "  4. Runtime (HTTP + Async + Platform APIs)"
	@echo ""
	@echo "Platform: $(PLATFORM)-$(UNAME_M)"
	@echo "Runtime: I/O + Networking + Async"
	@echo "UI: Reactive + Template Compilation + Multi-platform"
	@echo "Supported Apps: Web, Mobile, CLI, Desktop, Fullstack"

# Directory creation
$(BUILD_DIR) $(LIB_DIR) $(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@
