#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Def:
    pass

@dataclass
class LiveRange:
    pass

@dataclass
class LiveRangeAnalysis:
    pass

@dataclass
class Use:
    pass

class UseKind(IntEnum):
    """UseKind definition"""
    Read = 0
    Write = 1
    ReadWrite = 2


