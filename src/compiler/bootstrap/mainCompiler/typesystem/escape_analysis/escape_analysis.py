#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class DeviceState:
    pass

class MobileEscapeViolation(IntEnum):
    """MobileEscapeViolation definition"""
    BatteryConstrainedEscape = 0
    ThermalConstrainedEscape = 1
    PermissionDeniedEscape = 2
    PlatformIncompatibleEscape = 3
    CapabilityMissingEscape = 4


