#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class MobileDeviceProfile:
    pass

@dataclass
class MobileOptimizedCompiler:
    pass

@dataclass
class MobilePerformanceMetrics:
    pass

class MobileOptimizationTarget(IntEnum):
    """MobileOptimizationTarget definition"""
    Performance = 0
    BatteryLife = 1
    MemoryUsage = 2
    FrameRate = 3
    NetworkEfficiency = 4
    StartupTime = 5


