#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class CompiledFunction:
    pass

@dataclass
class JITEngine:
    pass

class OptLevel(IntEnum):
    """OptLevel definition"""
    O0 = 0
    O1 = 1
    O2 = 2
    O3 = 3


