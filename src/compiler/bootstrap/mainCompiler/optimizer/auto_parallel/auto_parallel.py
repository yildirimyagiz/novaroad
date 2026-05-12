#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AutoParallelizer:
    pass

@dataclass
class DependencyGraph:
    pass

@dataclass
class DependencyNode:
    pass

@dataclass
class LoopInfo:
    pass

@dataclass
class MobileParallelUtils:
    pass

@dataclass
class VarUse:
    pass

class ParallelismKind(IntEnum):
    """ParallelismKind definition"""
    SIMD = 0
    DataParallel = 1
    TaskParallel = 2
    Pipeline = 3
    NestedData = 4
    Recursive = 5
    UIParallel = 6
    AnimationParallel = 7
    MLBatchParallel = 8
    ARParallel = 9
    CameraPipeline = 10
    GestureParallel = 11
    SensorFusion = 12
    AccessibilityParallel = 13
    NetworkBatch = 14
    BackgroundTaskParallel = 15

class ParallelizationOpportunity(IntEnum):
    """ParallelizationOpportunity definition"""
    None = 16
    DataParallel = 17
    SIMD = 18
    Reduction = 19
    TaskParallel = 20
    UIParallel = 21
    AnimationParallel = 22
    MLBatchParallel = 23
    ARParallel = 24
    CameraPipeline = 25
    GestureParallel = 26
    SensorFusion = 27
    AccessibilityParallel = 28
    NetworkBatch = 29
    BackgroundTaskParallel = 30


