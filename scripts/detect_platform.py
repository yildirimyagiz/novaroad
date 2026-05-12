#!/usr/bin/env python3
"""
Platform detection utility for baseline selection.
Returns: "linux-x86_64", "darwin-arm64", etc.
"""
import platform
import sys

def detect_platform() -> str:
    """Detect platform identifier for baseline selection."""
    os_name = platform.system().lower()
    machine = platform.machine().lower()
    
    # Normalize architectures
    if machine in ("x86_64", "amd64"):
        arch = "x86_64"
    elif machine in ("aarch64", "arm64"):
        arch = "arm64"
    elif machine.startswith("arm"):
        arch = "arm"
    else:
        arch = machine
    
    return f"{os_name}-{arch}"

if __name__ == "__main__":
    print(detect_platform())
