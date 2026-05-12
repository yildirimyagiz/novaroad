#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class DeviceStateRequirements:
    pass

@dataclass
class MobileEffectConstraints:
    pass

@dataclass
class ResourceLimits:
    pass

class MobileEffectViolation(IntEnum):
    """MobileEffectViolation definition"""
    PermissionDenied = 0
    InsufficientBattery = 1
    ThermalConstraintViolated = 2
    PlatformMismatch = 3
    CapabilityUnavailable = 4
    ResourceLimitExceeded = 5
    DeviceStateIncompatible = 6


