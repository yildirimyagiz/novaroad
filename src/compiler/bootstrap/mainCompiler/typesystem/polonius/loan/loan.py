#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class FieldId:
    pass

@dataclass
class Loan:
    pass

@dataclass
class LoanContext:
    pass

@dataclass
class LoanId:
    pass

@dataclass
class LocalId:
    pass

class Path(IntEnum):
    """Path definition"""
    Local = 0
    Field = 1
    Index = 2
    Deref = 3


