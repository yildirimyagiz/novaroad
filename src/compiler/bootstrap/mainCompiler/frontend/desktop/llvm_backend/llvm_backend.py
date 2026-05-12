#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class CompileOptions:
    pass

@dataclass
class LLVMBackend:
    pass

class BackendError(IntEnum):
    """BackendError definition"""
    UnsupportedFeature = 0
    TypeMismatch = 1
    UndefinedSymbol = 2
    InternalError = 3


