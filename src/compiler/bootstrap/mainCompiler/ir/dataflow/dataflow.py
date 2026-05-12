#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class DataflowSolver:
    pass

class DataflowDirection(IntEnum):
    """DataflowDirection definition"""
    Forward = 0
    Backward = 1


