#!/bin/bash

echo "Creating convenient symlinks..."

# Create symlink for nova binary in root
ln -sf binaries/nova ./nova 2>/dev/null || echo "Symlink already exists or failed"

# Create symlink for bootstrap
ln -sf binaries/nova_bootstrap_stage0 ./nova_bootstrap_stage0 2>/dev/null

echo "Symlinks created!"
echo ""
echo "You can now use:"
echo "  ./nova         -> binaries/nova"
echo "  ./nova_bootstrap_stage0 -> binaries/nova_bootstrap_stage0"
