#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class MobileSimdContext:
    pass

@dataclass
class SimdCapability:
    pass

@dataclass
class f32x8:
    pass

@dataclass
class hexagon_f32x8:
    pass

@dataclass
class mongoose_f32x4:
    pass

@dataclass
class neon_f32x4:
    pass

@dataclass
class neural_f32x16:
    pass

class SimdArch(IntEnum):
    """SimdArch definition"""
    AVX512 = 0
    AVX2 = 1
    SSE4 = 2
    NEON = 3
    SVE = 4
    SVE2 = 5
    AMX = 6
    NeuralEngine = 7
    HexagonDSP = 8
    HVX = 9
    MongooseSIMD = 10
    NONE = 11

class SimdOperation(IntEnum):
    """SimdOperation definition"""
    Arithmetic = 12
    Matrix = 13
    Transform = 14
    Filter = 15
    Neural = 16
    Vision = 17
    Compression = 18


