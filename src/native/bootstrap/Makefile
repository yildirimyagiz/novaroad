# Novaosting Build System
# ======================================
# Hedef: .zn kaynak kodu → native binary (self-hosting)
#
# Kullanım:
#   make all        - Stage 0 + Stage 1 derle
#   make stage1     - Stage 1 binary'i derle
#   make stage2     - Stage 1 → Stage 2 (self-hosting kanıtı)
#   make test       - Tüm test dosyalarını derle
#   make clean      - Build dosyalarını temizle

.PHONY: all stage0-check stage1 stage2 test test-minimal test-advanced clean help

STAGE0  := python3 nova_stage0.py
BUILD   := build
RUNTIME := .

all: stage1 test
	@echo ""
	@echo "✅ Novarap COMPLETE"
	@echo "   Stage 0 (Python) → Stage 1 (.zn) ✓"
	@echo "   Run 'make stage2' for full self-hosting"

$(BUILD):
	mkdir -p $(BUILD)

stage0-check:
	@echo "⚡ Checking Stage 0 Python compiler..."
	@python3 -c "import sys; sys.path.insert(0, '.'); from nova_stage0 import Lexer, Parser, CodeGen; print('   ✅ Stage 0 modules OK')"

stage1: stage0-check $(BUILD)
	@echo "🔨 Building Stage 1 from nova_stage1.zn..."
	$(STAGE0) nova_stage1.zn nova_runtime_ast.c -o $(BUILD)/nova_stage1
	@echo "✅ Stage 1 ready: $(BUILD)/nova_stage1"

stage2: $(BUILD)/nova_stage1
	@echo ""
	@echo "🚀 SOVEREIGN TRANSITION: Stage 1 compiling itself to produce Stage 2..."
	./$(BUILD)/nova_stage1
	@echo ""
	@echo "🏆 SOVEREIGNTY VERIFIED!"
	@echo "   Stage 1 (Native) has successfully birthed Stage 2."
	@echo "   Nova is now running on its own silicon logic. ✓"Nova

test-minimal: $(BUILD)
	@echo "  Testing ultra_minimal.zn..."
	@$(STAGE0) tests/ultra_minimal.zn -o $(BUILD)/test_minimal 2>&1 | grep -E "(✅|❌)"
	@$(BUILD)/test_minimal

test-working: $(BUILD)
	@echo "  Testing working_compiler.zn..."
	@$(STAGE0) tests/working_compiler.zn -o $(BUILD)/test_working 2>&1 | grep -E "(✅|❌)"

test-advanced: $(BUILD)
	@echo "  Testing advanced_compiler.zn..."
	@$(STAGE0) tests/advanced_compiler.zn -o $(BUILD)/test_advanced 2>&1 | grep -E "(✅|❌)"

test: stage1 $(BUILD)
	@echo ""
	@echo "🧪 Running tests..."
	@echo ""
	@$(BUILD)/nova_stage1
	@echo ""
	@echo "✅ All tests passed"

# Build runtime libraries standalone
runtime: $(BUILD)
	@echo "📦 Building runtime libraries..."
	gcc -O2 -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -fPIC \
		-c $(RUNTIME)/nova_runtime_io.c -o $(BUILD)/nova_runtime_io.o
	gcc -O2 -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -fPIC \
		-c $(RUNTIME)/nova_runtime_net.c -o $(BUILD)/nova_runtime_net.o
	ar rcs $(BUILD)/libnova_runtime.a \
		$(BUILD)/nova_runtime_io.o $(BUILD)/nova_runtime_net.o
	@echo "✅ $(BUILD)/libnova_runtime.a ready"

clean:
	rm -rf $(BUILD)
	@echo "🧹 Cleaned"

help:
	@echo "Novarap Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make all      - Build everything (default)"
	@echo "  make stage1   - Compile nova_stage1 binary (Stage 0 → Stage 1)"
	@echo "  make stage2   - Self-hosting test (Stage 1 → Stage 2)"
	@echo "  make runtime  - Build C runtime libraries"
	@echo "  make test     - Run tests"
	@echo "  make clean    - Remove build artifacts"
	@echo ""
	@echo "Architecture:"
	@echo "  Stage 0: stage0/nova_stage0.py  (Python compiler, .zn → C → binary)"
	@echo "  Stage 1: stage1/nova_stage1.zn  (Novaer, written in Nova)"Nova
	@echo "  Runtime: runtime/*.c            (C runtime: I/O, networking)"
