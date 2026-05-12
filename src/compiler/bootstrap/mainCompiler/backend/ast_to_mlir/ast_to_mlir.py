#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ASTToMLIRLowering:
    pass

@dataclass
class MLIRBlock:
    pass

@dataclass
class MLIRFunction:
    pass

@dataclass
class MLIRModule:
    pass

class MLIRType(IntEnum):
    """MLIRType definition"""
    I64 = 0
    F64 = 1
    Bool = 2
    Linear = 3
    Borrowed = 4
    Owned = 5

class MLIROperation(IntEnum):
    """MLIROperation definition"""
    Move = 6
    Borrow = 7
    Drop = 8
    Call = 9
    PureCall = 10
    WithEffect = 11
    Verify = 12
    VerifiedBlock = 13

class EffectSet(IntEnum):
    """EffectSet definition"""
    PURE = 14
    IO = 15
    MEMORY = 16
    UNKNOWN = 17


