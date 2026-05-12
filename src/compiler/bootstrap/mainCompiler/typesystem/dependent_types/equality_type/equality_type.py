#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class EqualityReasoning:
    pass

@dataclass
class EqualityType:
    pass

@dataclass
class MobileEqualityContext:
    pass

class EqualityProof(IntEnum):
    """EqualityProof definition"""
    Refl = 0
    Sym = 1
    Trans = 2
    Cong = 3
    Beta = 4
    Delta = 5
    MobilePermission = 6
    MobileCapability = 7
    BatteryAware = 8
    ThermalAware = 9


