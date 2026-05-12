#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class TypeChecker:
    pass

class TypeKind(IntEnum):
    """TypeKind definition"""
    I64 = 0
    F64 = 1
    Bool = 2
    Str = 3
    Void = 4
    Refinement = 5
    Function = 6
    CameraFrame = 7
    AudioBuffer = 8
    LocationCoords = 9
    SensorData = 10
    NeuralTensor = 11
    DSP_Signal = 12
    TouchEvent = 13
    DisplayBuffer = 14
    BatteryStatus = 15
    ThermalReading = 16
    PermissionToken = 17
    HardwareHandle = 18
    BatteryConstrained = 19
    ThermalConstrained = 20
    PermissionRequired = 21
    PlatformSpecific = 22
    CapabilityRequired = 23
    Unknown = 24


