#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class IRParam:
    pass

class CoreIR(IntEnum):
    """CoreIR definition"""
    Func = 0
    When = 1
    Loop = 2
    Give = 3
    BinOp = 4
    UnOp = 5
    Call = 6
    Load = 7
    Store = 8
    Literal = 9
    Alloc = 10
    ComputeBlock = 11

class IRType(IntEnum):
    """IRType definition"""
    Void = 12
    Bool = 13
    I64 = 14
    F64 = 15
    Str = 16
    Ptr = 17
    Vec = 18
    Tensor = 19
    Shape = 20

class IRLit(IntEnum):
    """IRLit definition"""
    Bool = 21
    I64 = 22
    F64 = 23
    Str = 24

class IROp(IntEnum):
    """IROp definition"""
    Add = 25
    Sub = 26
    Mul = 27
    Div = 28
    Mod = 29
    Eq = 30
    Ne = 31
    Lt = 32
    Gt = 33
    Le = 34
    Ge = 35
    And = 36
    Or = 37
    Not = 38
    BitAnd = 39
    BitOr = 40
    BitXor = 41


