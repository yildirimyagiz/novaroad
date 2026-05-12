#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class MobileOptimizationStats:
    pass

@dataclass
class Optimizer:
    pass

class OptimizeLevel(IntEnum):
    """OptimizeLevel definition"""
    None = 0
    Standard = 1
    Supreme = 2
    Mobile = 3
    Ultimate = 4

class MobileOptimizationTarget(IntEnum):
    """MobileOptimizationTarget definition"""
    Performance = 5
    BatteryLife = 6
    FrameRate = 7
    NetworkEfficiency = 8
    StartupTime = 9
    ThermalManagement = 10


