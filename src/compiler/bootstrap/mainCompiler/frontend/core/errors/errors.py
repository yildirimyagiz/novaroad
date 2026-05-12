#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class CodeGenError:
    pass

@dataclass
class Diagnostic:
    pass

@dataclass
class ErrorReporter:
    pass

@dataclass
class LexError:
    pass

@dataclass
class LintWarning:
    pass

@dataclass
class ParseError:
    pass

@dataclass
class RuntimeError:
    pass

@dataclass
class SemanticError:
    pass

@dataclass
class TypeError:
    pass

class CompilerError(IntEnum):
    """CompilerError definition"""
    Lexical = 0
    Syntax = 1
    Semantic = 2
    Type = 3
    Symbol = 4
    CodeGen = 5
    Runtime = 6

class SemanticErrorKind(IntEnum):
    """SemanticErrorKind definition"""
    UnreachableCode = 7
    UnusedVariable = 8
    UnusedFunction = 9
    MissingReturn = 10
    DeadCode = 11
    InfiniteLoop = 12
    ContractViolation = 13
    UnsafeOperation = 14
    DeprecatedUsage = 15

class SymbolKind(IntEnum):
    """SymbolKind definition"""
    Variable = 16
    Function = 17
    DataType = 18
    Trait = 19
    Module = 20
    Field = 21
    Method = 22
    Constant = 23
    TypeAlias = 24
    Macro = 25

class SymbolError(IntEnum):
    """SymbolError definition"""
    NotFound = 26
    AlreadyDefined = 27
    WrongKind = 28
    NotAccessible = 29
    CircularReference = 30

class CodeGenPhase(IntEnum):
    """CodeGenPhase definition"""
    IRGeneration = 31
    Optimization = 32
    TargetCodeEmission = 33
    Linking = 34

class Severity(IntEnum):
    """Severity definition"""
    Error = 35
    Warning = 36
    Info = 37
    Hint = 38


