#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class CrossBackend:
    pass

@dataclass
class Target:
    pass

class Arch(IntEnum):
    """Arch definition"""
    X86_64 = 0
    AArch64 = 1
    ARM = 2
    RISCV64 = 3
    WASM32 = 4

class OS(IntEnum):
    """OS definition"""
    Linux = 5
    Windows = 6
    MacOS = 7
    iOS = 8
    Android = 9
    FreeBSD = 10
    None = 11

class Environment(IntEnum):
    """Environment definition"""
    GNU = 12
    MSVC = 13
    Musl = 14
    Android = 15
    Darwin = 16
    None = 17


