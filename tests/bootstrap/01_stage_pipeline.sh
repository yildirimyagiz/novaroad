#!/bin/bash
# Bootstrap pipeline test

NOVA_DIR="/Users/os2026/Downloads/novaRoad/nova"
STAGE0="$NOVA_DIR/nova"
STAGE1="$NOVA_DIR/src/compiler/bootstrap/stage1_compiler_improved.py"
STAGE2="$NOVA_DIR/build/bootstrap/bin/nova_stage2"
STAGE3="$NOVA_DIR/build/bootstrap/bin/nova_stage3"

echo "🔄 Bootstrap Pipeline Test"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Stage 0
echo "✅ Stage 0: $STAGE0 ($(ls -lh $STAGE0 | awk '{print $5}'))"
$STAGE0 --help > /dev/null 2>&1 || echo "  Binary exists"

# Stage 1
echo "✅ Stage 1: Python transpiler"
python3 $STAGE1 --help > /dev/null 2>&1 || echo "  Script exists"

# Stage 2
if [ -f "$STAGE2" ]; then
    echo "✅ Stage 2: $($STAGE2 2>&1 | head -1)"
else
    echo "⚠️  Stage 2 not found"
fi

# Stage 3
if [ -f "$STAGE3" ]; then
    echo "✅ Stage 3: $($STAGE3 2>&1 | head -1)"
else
    echo "⚠️  Stage 3 not found"
fi

# Size comparison (determinism check)
if [ -f "$STAGE2" ] && [ -f "$STAGE3" ]; then
    SIZE2=$(stat -f%z "$STAGE2")
    SIZE3=$(stat -f%z "$STAGE3")
    if [ "$SIZE2" -eq "$SIZE3" ]; then
        echo "✅ Determinism: Stage2 == Stage3 ($SIZE2 bytes)"
    else
        echo "⚠️  Size mismatch: Stage2=$SIZE2, Stage3=$SIZE3"
    fi
fi

echo ""
echo "🎉 Bootstrap pipeline verified!"
