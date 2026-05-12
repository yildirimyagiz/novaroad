#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class MobileContext:
    pass

@dataclass
class PoloniusContext:
    pass

@dataclass
class PoloniusStats:
    pass

class ThermalState(IntEnum):
    """ThermalState definition"""
    Cool = 0
    Warm = 1
    Hot = 2
    Critical = 3

class Platform(IntEnum):
    """Platform definition"""
    iOS = 4
    Android = 5
    Desktop = 6
    Server = 7


