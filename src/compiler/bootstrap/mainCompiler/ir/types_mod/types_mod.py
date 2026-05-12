#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class TypeContext:
    pass

class IRType(IntEnum):
    """IRType definition"""
    Void = 0
    Bool = 1
    I8 = 2
    I16 = 3
    I32 = 4
    I64 = 5
    U8 = 6
    U16 = 7
    U32 = 8
    U64 = 9
    F16 = 10
    F32 = 11
    F64 = 12
    Ptr = 13
    Array = 14
    Vector = 15
    Struct = 16
    Function = 17
    Tensor = 18
    Named = 19


