#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class MobileApp:
    pass

@dataclass
class Navigator:
    pass

class Platform(IntEnum):
    """Platform definition"""
    iOS = 0
    Android = 1
    Unknown = 2


