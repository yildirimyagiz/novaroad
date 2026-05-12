#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ParseError:
    pass

@dataclass
class Span:
    pass

@dataclass
class Token:
    pass

class ParseResult(IntEnum):
    """ParseResult definition"""
    Success = 0
    Failure = 1

class TokenKind(IntEnum):
    """TokenKind definition"""
    Fn = 2
    Let = 3
    If = 4
    Else = 5
    While = 6
    For = 7
    Match = 8
    Return = 9
    Plus = 10
    Minus = 11
    Star = 12
    Slash = 13
    Eq = 14
    EqEq = 15
    Lt = 16
    Gt = 17
    LParen = 18
    RParen = 19
    LBrace = 20
    RBrace = 21
    LBracket = 22
    RBracket = 23
    Comma = 24
    Semicolon = 25
    Colon = 26
    Arrow = 27
    IntLit = 28
    StringLit = 29
    BoolLit = 30
    Ident = 31
    Eof = 32


