#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class JITBackend:
    pass

@dataclass
class JITCompiler:
    pass

@dataclass
class JITFunction:
    pass

@dataclass
class JITRuntime:
    pass

class JITMode(IntEnum):
    """JITMode definition"""
    Lazy = 0
    Eager = 1
    Adaptive = 2

class OptimizationLevel(IntEnum):
    """OptimizationLevel definition"""
    NoOptimization = 3
    BasicOptimization = 4
    StandardOptimization = 5
    AggressiveOptimization = 6

class TargetArch(IntEnum):
    """TargetArch definition"""
    X86_64 = 7
    ARM64 = 8
    RISCV64 = 9

class CompilationStatus(IntEnum):
    """CompilationStatus definition"""
    NotCompiled = 10
    Compiling = 11
    Compiled = 12
    Failed = 13

class OptLevel(IntEnum):
    """OptLevel definition"""
    None = 14
    Speed = 15
    SpeedAndSize = 16


