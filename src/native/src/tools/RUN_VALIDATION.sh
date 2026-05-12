#!/bin/bash
# Nova Scientific Validation Suite - Quick Run Script

set -e

echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║  Nova Scientific Performance & Efficiency Validation Suite             ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Build if needed
if [ ! -f nova_scientific_validation ]; then
    echo "🔨 Building validation suite..."
    make -f Makefile.validation
    echo ""
fi

# Run
echo "🚀 Running validation suite..."
echo "   (This will take 2-5 minutes on modern hardware)"
echo ""

./nova_scientific_validation

echo ""
echo "✅ Validation complete!"
