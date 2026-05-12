#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ValueContext:
    pass

class Value(IntEnum):
    """Value definition"""
    Register = 0
    Const = 1
    Argument = 2
    Global = 3
    Undef = 4

class Constant(IntEnum):
    """Constant definition"""
    Int = 5
    UInt = 6
    Float = 7
    Bool = 8
    Null = 9
    String = 10
    Array = 11
    Struct = 12
    Zeroinitializer = 13


