#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class LoopInfo:
    pass

@dataclass
class SIMDBackend:
    pass

class SIMDISASet(IntEnum):
    """SIMDISASet definition"""
    SSE4_2 = 0
    AVX = 1
    AVX2 = 2
    AVX512 = 3
    NEON = 4
    SVE = 5


