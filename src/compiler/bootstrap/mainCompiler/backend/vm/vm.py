#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class CallFrame:
    pass

@dataclass
class NovaVM:
    pass

@dataclass
class VM:
    pass

class Value(IntEnum):
    """Value definition"""
    Int = 0
    Float = 1
    Bool = 2
    String = 3
    Object = 4
    Null = 5


