#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class BasicBlock:
    pass

@dataclass
class CFG:
    pass

@dataclass
class CFGBuilder:
    pass

@dataclass
class DominanceInfo:
    pass

@dataclass
class Edge:
    pass

class EdgeKind(IntEnum):
    """EdgeKind definition"""
    Normal = 0
    Conditional = 1
    Exception = 2
    Return = 3


