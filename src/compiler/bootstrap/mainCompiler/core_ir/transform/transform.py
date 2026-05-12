#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class TransformEngine:
    pass

class MIRPass(IntEnum):
    """MIRPass definition"""
    ConstProp = 0
    DeadCodeElim = 1
    HarmonicUnroll = 2
    SIMDVectorize = 3
    Inlining = 4


