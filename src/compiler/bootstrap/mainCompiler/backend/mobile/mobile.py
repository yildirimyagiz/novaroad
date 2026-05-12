#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class MobileBackend:
    pass

@dataclass
class MobilePackage:
    pass

@dataclass
class MobileTarget:
    pass

@dataclass
class ProjectFile:
    pass

class MobilePlatform(IntEnum):
    """MobilePlatform definition"""
    iOS = 0
    Android = 1


