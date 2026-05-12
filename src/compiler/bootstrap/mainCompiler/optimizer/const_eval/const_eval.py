#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ConstEvalError:
    pass

@dataclass
class ConstEvaluator:
    pass

@dataclass
class ConstFunction:
    pass

class ConstValue(IntEnum):
    """ConstValue definition"""
    Int = 0
    Float = 1
    Bool = 2
    Str = 3
    Array = 4
    Struct = 5
    Point = 6
    TouchEvent = 7
    GestureEvent = 8
    SensorData = 9
    UIComponent = 10
    Color = 11
    Size = 12
    Null = 13


