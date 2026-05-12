#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class DeviceState:
    pass

@dataclass
class Lifetime:
    pass

@dataclass
class LifetimeInferencer:
    pass

@dataclass
class Reference:
    pass

class LifetimeRelation(IntEnum):
    """LifetimeRelation definition"""
    Outlives = 0
    Equals = 1
    Contains = 2

class MobileLifetimeViolation(IntEnum):
    """MobileLifetimeViolation definition"""
    BatteryConstrainedInference = 3
    ThermalConstrainedInference = 4
    PermissionDeniedInference = 5
    PlatformIncompatibleInference = 6
    CapabilityMissingInference = 7


