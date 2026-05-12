#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class EffectConstraintSet:
    pass

@dataclass
class EffectVar:
    pass

class Effect(IntEnum):
    """Effect definition"""
    Pure = 0
    IO = 1
    Diverge = 2
    Panic = 3
    Async = 4
    Custom = 5
    Var = 6
    Union = 7

class EffectConstraint(IntEnum):
    """EffectConstraint definition"""
    Eq = 8
    Sub = 9
    Bound = 10


