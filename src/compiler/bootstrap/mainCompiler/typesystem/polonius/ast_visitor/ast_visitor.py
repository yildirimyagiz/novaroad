#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ASTVisitor:
    pass

@dataclass
class FunctionParam:
    pass

@dataclass
class MatchArm:
    pass

class Expr(IntEnum):
    """Expr definition"""
    Block = 0
    If = 1
    Match = 2
    While = 3
    For = 4
    Let = 5
    Assign = 6
    Borrow = 7
    Call = 8
    Field = 9
    Index = 10
    Var = 11
    Literal = 12

class Stmt(IntEnum):
    """Stmt definition"""
    Expr = 13
    Let = 14
    Return = 15

class Pattern(IntEnum):
    """Pattern definition"""
    Wildcard = 16
    Var = 17
    Literal = 18

class Literal(IntEnum):
    """Literal definition"""
    Int = 19
    Bool = 20
    String = 21

class Type(IntEnum):
    """Type definition"""
    Int = 22
    Bool = 23
    String = 24
    Unit = 25


