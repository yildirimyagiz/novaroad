#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Scope:
    pass

@dataclass
class SymbolEntry:
    pass

@dataclass
class SymbolInfo:
    pass

@dataclass
class SymbolTable:
    pass

class Visibility(IntEnum):
    """Visibility definition"""
    Private = 0
    Public = 1
    Open = 2

class ScopeKind(IntEnum):
    """ScopeKind definition"""
    Global = 3
    Module = 4
    Function = 5
    Block = 6
    Loop = 7
    MatchArm = 8


