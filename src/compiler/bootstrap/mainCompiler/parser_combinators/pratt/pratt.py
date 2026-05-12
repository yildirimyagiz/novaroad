#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class InfixOp:
    pass

@dataclass
class PostfixOp:
    pass

@dataclass
class PrattParser:
    pass

@dataclass
class PrefixOp:
    pass

class Associativity(IntEnum):
    """Associativity definition"""
    Left = 0
    Right = 1

class ArithExpr(IntEnum):
    """ArithExpr definition"""
    Num = 2
    Var = 3
    Add = 4
    Sub = 5
    Mul = 6
    Div = 7
    Neg = 8


