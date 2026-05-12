#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ImportContext:
    pass

@dataclass
class ModuleInfo:
    pass

@dataclass
class ModuleResolver:
    pass

class ModuleError(IntEnum):
    """ModuleError definition"""
    ModuleNotFound = 0
    ItemNotExported = 1
    CircularDependency = 2
    DuplicateModule = 3
    InvalidModulePath = 4


