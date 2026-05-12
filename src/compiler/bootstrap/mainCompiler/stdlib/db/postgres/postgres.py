#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class PgConfig:
    pass

@dataclass
class PgConnection:
    pass

@dataclass
class PgResult:
    pass

@dataclass
class PgRow:
    pass

class PgValue(IntEnum):
    """PgValue definition"""
    Null = 0
    Bool = 1
    Int = 2
    Float = 3
    Text = 4
    Bytes = 5


