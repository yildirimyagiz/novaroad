#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class CombinedOptimizer:
    pass

@dataclass
class LinkTimeOptimizer:
    pass

@dataclass
class MobileProfileData:
    pass

@dataclass
class OptimizationDecision:
    pass

@dataclass
class ProfileData:
    pass

@dataclass
class ProfileGuidedOptimizer:
    pass

class OptimizationLevel(IntEnum):
    """OptimizationLevel definition"""
    O0 = 0
    O1 = 1
    O2 = 2
    O3 = 3
    O4 = 4
    Pgo = 5
    Lto = 6
    MobilePgo = 7
    MobileLto = 8
    UltimateMobile = 9

class MobileOptimizationLevel(IntEnum):
    """MobileOptimizationLevel definition"""
    Standard = 10
    Performance = 11
    Battery = 12
    Balanced = 13


