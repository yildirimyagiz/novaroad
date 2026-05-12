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
class LifetimeAnnotationParser:
    pass

@dataclass
class LifetimeConstraint:
    pass

@dataclass
class LifetimeElision:
    pass

class LifetimeStyle(IntEnum):
    """LifetimeStyle definition"""
    Inferred = 0
    Explicit = 1
    Elided = 2

class MobileLifetimeViolation(IntEnum):
    """MobileLifetimeViolation definition"""
    BatteryConstrainedLifetime = 3
    ThermalConstrainedLifetime = 4
    PermissionDeniedLifetime = 5
    PlatformIncompatibleLifetime = 6
    CapabilityMissingLifetime = 7
    LifetimeTooLong = 8


