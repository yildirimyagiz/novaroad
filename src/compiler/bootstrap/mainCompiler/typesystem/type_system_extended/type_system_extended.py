#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class TypeCheckerExtended:
    pass

class TypeKindExtended(IntEnum):
    """TypeKindExtended definition"""
    I32 = 0
    I64 = 1
    F32 = 2
    F64 = 3
    Bool = 4
    Str = 5
    Void = 6
    Unknown = 7
    Array = 8
    Struct = 9
    Function = 10
    Generic = 11
    Pi = 12
    Sigma = 13
    Equality = 14
    Refinement = 15
    Universe = 16
    Meta = 17
    MobileResource = 18
    BatteryDependent = 19
    ThermalDependent = 20
    PermissionDependent = 21
    PlatformDependent = 22
    CapabilityDependent = 23


