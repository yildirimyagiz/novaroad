#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class REPL:
    pass

class Value(IntEnum):
    """Value definition"""
    Int = 0
    Float = 1
    Bool = 2
    String = 3
    Unit = 4


