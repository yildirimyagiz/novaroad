#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ExistentialRegion:
    pass

@dataclass
class OriginContext:
    pass

@dataclass
class PlaceholderRegion:
    pass

@dataclass
class UniversalRegion:
    pass

class Origin(IntEnum):
    """Origin definition"""
    Universal = 0
    Existential = 1
    Placeholder = 2


