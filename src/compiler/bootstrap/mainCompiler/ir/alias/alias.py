#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AliasAnalysis:
    pass

class AliasResult(IntEnum):
    """AliasResult definition"""
    NoAlias = 0
    MayAlias = 1
    MustAlias = 2
    PartialAlias = 3


