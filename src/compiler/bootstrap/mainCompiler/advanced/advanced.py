#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class EventBus:
    pass

@dataclass
class NovaRuntime:
    pass

@dataclass
class PlatformCapabilities:
    pass

@dataclass
class RuntimeConfig:
    pass

@dataclass
class RuntimeStatus:
    pass

class LogLevel(IntEnum):
    """LogLevel definition"""
    Silent = 0
    Error = 1
    Warn = 2
    Info = 3
    Debug = 4
    Trace = 5

class RuntimeState(IntEnum):
    """RuntimeState definition"""
    Initializing = 6
    Running = 7
    Suspended = 8
    Terminating = 9
    Error = 10

class RuntimeEvent(IntEnum):
    """RuntimeEvent definition"""
    Suspend = 11
    Resume = 12
    Terminate = 13
    MemoryWarning = 14
    AccessibilityChanged = 15
    ThemeChanged = 16
    OrientationChanged = 17
    NetworkChanged = 18
    Custom = 19

class Orientation(IntEnum):
    """Orientation definition"""
    Portrait = 20
    Landscape = 21
    PortraitUpsideDown = 22
    LandscapeRight = 23

class NetworkStatus(IntEnum):
    """NetworkStatus definition"""
    Online = 24
    Offline = 25
    Cellular = 26
    WiFi = 27

class RuntimeError(IntEnum):
    """RuntimeError definition"""
    InitFailed = 28
    GPUInitFailed = 29
    MLInitFailed = 30
    UIInitFailed = 31
    AccessibilityInitFailed = 32
    UnknownError = 33


