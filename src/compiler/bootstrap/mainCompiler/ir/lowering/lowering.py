#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class LoweringContext:
    pass

class ASTNode(IntEnum):
    """ASTNode definition"""
    VarDecl = 0
    Assignment = 1
    Return = 2
    If = 3
    While = 4
    Block = 5
    IntLiteral = 6
    FloatLiteral = 7
    BoolLiteral = 8
    Variable = 9
    BinaryOp = 10
    Call = 11

class ASTType(IntEnum):
    """ASTType definition"""
    Int = 12
    Float = 13
    Bool = 14
    Void = 15


