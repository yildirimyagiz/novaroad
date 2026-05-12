#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class BasicBlock:
    pass

@dataclass
class LocalDecl:
    pass

@dataclass
class MIRFunction:
    pass

@dataclass
class MIRProgram:
    pass

@dataclass
class Place:
    pass

class MIRStmt(IntEnum):
    """MIRStmt definition"""
    Assign = 0
    Call = 1
    Nop = 2

class MIRTerminator(IntEnum):
    """MIRTerminator definition"""
    Return = 3
    Goto = 4
    Branch = 5
    Unreachable = 6

class Operand(IntEnum):
    """Operand definition"""
    Copy = 7
    Move = 8
    Const = 9

class MIRExpr(IntEnum):
    """MIRExpr definition"""
    Use = 10
    BinOp = 11
    UnOp = 12


