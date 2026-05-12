#!/bin/bash
# Fix missing headers in compiler files

echo "🔧 Fixing header includes in compiler files..."
echo ""

SRC="src/compiler"

# Add stdbool.h to files that need it
for file in ${SRC}/*.c; do
    if [ -f "$file" ]; then
        # Check if already has stdbool.h
        if ! grep -q "#include <stdbool.h>" "$file"; then
            # Add it after other includes or at the top
            sed -i '' '1i\
#include <stdbool.h>\
' "$file" 2>/dev/null || true
            echo "   ✓ Added stdbool.h to $(basename $file)"
        fi
    fi
done

echo ""
echo "✅ Headers fixed!"
echo ""

