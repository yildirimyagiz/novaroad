#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Kernel:
    pass

@dataclass
class KernelParam:
    pass

@dataclass
class MetalBackend:
    pass

class MetalDevice(IntEnum):
    """MetalDevice definition"""
    MacOS = 0
    iOS = 1
    iPadOS = 2


