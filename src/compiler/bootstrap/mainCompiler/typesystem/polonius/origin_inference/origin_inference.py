#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class OriginConstraint:
    pass

@dataclass
class OriginInference:
    pass

class ConstraintKind(IntEnum):
    """ConstraintKind definition"""
    Outlives = 0
    PathHasOrigin = 1
    CallPropagate = 2


