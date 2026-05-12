#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class CrossCompiler:
    pass

class Target(IntEnum):
    """Target definition"""
    X86_64_Linux = 0
    X86_64_Windows = 1
    X86_64_MacOS = 2
    ARM64_Linux = 3
    ARM64_MacOS = 4
    WASM32 = 5
    RISCV64 = 6


