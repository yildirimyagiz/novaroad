#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class DeviceState:
    pass

@dataclass
class DropCheckResult:
    pass

@dataclass
class DropChecker:
    pass

@dataclass
class DropPoint:
    pass

@dataclass
class MobileDropContext:
    pass

@dataclass
class ScopeInfo:
    pass

class DropStrategy(IntEnum):
    """DropStrategy definition"""
    None = 0
    Trivial = 1
    Destructor = 2
    Recursive = 3
    MobileResourceCleanup = 4
    DeferredDrop = 5
    ConditionalDrop = 6
    PlatformSpecificDrop = 7
    ThermalAwareDrop = 8
    PermissionAwareDrop = 9

class MobileResourceType(IntEnum):
    """MobileResourceType definition"""
    Camera = 10
    Microphone = 11
    Location = 12
    Accelerometer = 13
    Gyroscope = 14
    NeuralEngine = 15
    DSP = 16
    GPS = 17
    Bluetooth = 18
    WiFi = 19
    Cellular = 20
    NFC = 21
    TouchScreen = 22
    Display = 23
    Storage = 24

class DropIntensity(IntEnum):
    """DropIntensity definition"""
    Low = 25
    Moderate = 26
    High = 27
    Critical = 28

class MobileDropViolation(IntEnum):
    """MobileDropViolation definition"""
    PermissionDeniedForDrop = 29
    InsufficientBatteryForDrop = 30
    ThermalConstraintForDrop = 31
    PlatformMismatchForDrop = 32
    CapabilityUnavailableForDrop = 33
    DropTooIntensiveForBattery = 34
    DropTooIntensiveForThermal = 35


