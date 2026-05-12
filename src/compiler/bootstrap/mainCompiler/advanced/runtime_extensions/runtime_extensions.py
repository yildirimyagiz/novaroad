#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AnimationEngine:
    pass

@dataclass
class PerformanceMetrics:
    pass

@dataclass
class PlatformBridge:
    pass

@dataclass
class PlatformCapabilities:
    pass

class LogLevel(IntEnum):
    """LogLevel definition"""
    Silent = 0
    Error = 1
    Warn = 2
    Info = 3
    Debug = 4
    Trace = 5


