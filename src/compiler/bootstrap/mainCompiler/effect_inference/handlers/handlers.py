#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Continuation:
    pass

@dataclass
class EffectHandler:
    pass

@dataclass
class HandlerCase:
    pass

class Value(IntEnum):
    """Value definition"""
    Int = 0
    String = 1
    Unit = 2


