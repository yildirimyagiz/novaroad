#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ConstRegistry:
    pass

@dataclass
class File:
    pass

@dataclass
class PhantomData:
    pass

@dataclass
class Refined:
    pass

@dataclass
class Sigma:
    pass

@dataclass
class SizedVec:
    pass

@dataclass
class TypeEnv:
    pass

class Nat(IntEnum):
    """Nat definition"""
    Zero = 0
    Succ = 1

class TBool(IntEnum):
    """TBool definition"""
    True = 2
    False = 3

class TypeList(IntEnum):
    """TypeList definition"""
    Nil = 4
    Cons = 5

class TypeLevelValue(IntEnum):
    """TypeLevelValue definition"""
    Nat = 6
    Bool = 7
    Str = 8
    List = 9
    Tuple = 10
    Fn = 11
    Unit = 12
    Error = 13

class TypeExpr(IntEnum):
    """TypeExpr definition"""
    Prim = 14
    Var = 15
    App = 16
    Fn = 17
    Tuple = 18
    Const = 19
    Add = 20
    Sub = 21
    Mul = 22
    If = 23

class FileState(IntEnum):
    """FileState definition"""
    Closed = 24
    Opened = 25
    Locked = 26

class FileMode(IntEnum):
    """FileMode definition"""
    ReadOnly = 27
    WriteOnly = 28
    ReadWrite = 29

class FileError(IntEnum):
    """FileError definition"""
    NotFound = 30
    PermissionDenied = 31
    AlreadyOpen = 32
    NotOpen = 33
    IoError = 34


