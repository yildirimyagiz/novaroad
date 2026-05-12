#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ASTToMLIRLowering:
    pass

@dataclass
class MLIRModule:
    pass

@dataclass
class MLIROperation:
    pass

class MLIRDialect(IntEnum):
    """MLIRDialect definition"""
    Arith = 0
    SCF = 1
    Func = 2
    Linalg = 3
    Vector = 4
    GPU = 5
    Nova = 6


