#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class BorrowCheckResult:
    pass

@dataclass
class BorrowChecker:
    pass

@dataclass
class MobileBorrowContext:
    pass

class Ownership(IntEnum):
    """Ownership definition"""
    Owned = 0
    BorrowedShared = 1
    BorrowedMut = 2
    Moved = 3
    Dropped = 4
    BorrowedMobileShared = 5
    BorrowedMobileExclusive = 6
    BorrowedWithPermission = 7
    BorrowedBatteryAware = 8
    BorrowedThermalAware = 9
    BorrowedCapabilityAware = 10

class MobileResource(IntEnum):
    """MobileResource definition"""
    Camera = 11
    Microphone = 12
    Location = 13
    Accelerometer = 14
    Gyroscope = 15
    NeuralEngine = 16
    DSP = 17
    GPS = 18
    Bluetooth = 19
    WiFi = 20
    Cellular = 21
    NFC = 22
    TouchScreen = 23
    Display = 24
    Storage = 25

class BorrowIntensity(IntEnum):
    """BorrowIntensity definition"""
    Low = 26
    Moderate = 27
    High = 28
    Critical = 29

class BorrowDuration(IntEnum):
    """BorrowDuration definition"""
    Instant = 30
    Short = 31
    Medium = 32
    Long = 33
    Extended = 34
    Continuous = 35

class MobileBorrowViolation(IntEnum):
    """MobileBorrowViolation definition"""
    PermissionDenied = 36
    InsufficientBattery = 37
    CapabilityUnavailable = 38
    PlatformMismatch = 39
    ThermalConstraint = 40
    ResourceConflict = 41
    BatteryIntensiveBorrow = 42
    ThermalIntensiveBorrow = 43


