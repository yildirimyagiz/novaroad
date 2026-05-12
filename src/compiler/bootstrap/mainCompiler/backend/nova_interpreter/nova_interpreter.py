#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Interpreter:
    pass

class Value(IntEnum):
    """Value definition"""
    Integer = 0
    Float = 1
    Boolean = 2
    String = 3
    Function = 4
    Null = 5


