#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Condition:
    pass

@dataclass
class NullableInfo:
    pass

@dataclass
class PathAnalysis:
    pass

@dataclass
class Predicate:
    pass

class ConditionKind(IntEnum):
    """ConditionKind definition"""
    IsTrue = 0
    IsFalse = 1
    Equals = 2
    NotEquals = 3
    IsSome = 4
    IsNone = 5

class PredicateKind(IntEnum):
    """PredicateKind definition"""
    NonNull = 6
    Initialized = 7
    Moved = 8
    Borrowed = 9


