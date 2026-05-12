#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AnyType:
    pass

@dataclass
class ArrayType:
    pass

@dataclass
class ConstType:
    pass

@dataclass
class DependentType:
    pass

@dataclass
class DimensionExpr:
    pass

@dataclass
class DynRulesType:
    pass

@dataclass
class EffectType:
    pass

@dataclass
class EnumType:
    pass

@dataclass
class FlowType:
    pass

@dataclass
class FnPtrType:
    pass

@dataclass
class GenericType:
    pass

@dataclass
class HigherKindedType:
    pass

@dataclass
class InferType:
    pass

@dataclass
class IntersectionType:
    pass

@dataclass
class Lifetime:
    pass

@dataclass
class LifetimeType:
    pass

@dataclass
class NeverType:
    pass

@dataclass
class OpaqueType:
    pass

@dataclass
class PathType:
    pass

@dataclass
class PhantomType:
    pass

@dataclass
class PlatformType:
    pass

@dataclass
class QtyType:
    pass

@dataclass
class RefType:
    pass

@dataclass
class StructType:
    pass

@dataclass
class SymbolicType:
    pass

@dataclass
class TensorType:
    pass

@dataclass
class TupleType:
    pass

@dataclass
class UnionType:
    pass

@dataclass
class VPUType:
    pass

class PrimitiveType(IntEnum):
    """PrimitiveType definition"""
    I8 = 0
    I16 = 1
    I32 = 2
    I64 = 3
    I128 = 4
    U8 = 5
    U16 = 6
    U32 = 7
    U64 = 8
    U128 = 9
    F16 = 10
    F32 = 11
    F64 = 12
    Bool = 13
    Char = 14
    Str = 15
    Unit = 16
    Never = 17

class GenericArg(IntEnum):
    """GenericArg definition"""
    Type = 18
    Const = 19
    Lifetime = 20

class ShapeDim(IntEnum):
    """ShapeDim definition"""
    Named = 21
    Const = 22
    Dynamic = 23
    Symbolic = 24

class Effect(IntEnum):
    """Effect definition"""
    IO = 25
    Async = 26
    State = 27
    Exception = 28
    Nondet = 29
    Custom = 30

class ConstKind(IntEnum):
    """ConstKind definition"""
    Usize = 31
    I64 = 32
    Bool = 33

class SymbolicExpr(IntEnum):
    """SymbolicExpr definition"""
    Var = 34
    Add = 35
    Mul = 36
    Const = 37

class Platform(IntEnum):
    """Platform definition"""
    IOS = 38
    Android = 39
    Web = 40
    Desktop = 41
    Embedded = 42

class DesktopOS(IntEnum):
    """DesktopOS definition"""
    Windows = 43
    MacOS = 44
    Linux = 45

class TypeExpr(IntEnum):
    """TypeExpr definition"""
    Prim = 46
    Ref = 47
    Generic = 48
    Path = 49
    Tuple = 50
    Array = 51
    Struct = 52
    Enum = 53
    Qty = 54
    Tensor = 55
    Flow = 56
    FnPtr = 57
    DynRules = 58
    Union = 59
    Intersection = 60
    Phantom = 61
    Effect = 62
    Lifetime = 63
    Const = 64
    Dependent = 65
    HigherKinded = 66
    Opaque = 67
    Symbolic = 68
    Platform = 69
    VPU = 70
    Infer = 71
    Never = 72
    Any = 73
    Var = 74

class ConstExpr(IntEnum):
    """ConstExpr definition"""
    Int = 75
    Bool = 76
    Str = 77
    Add = 78
    Mul = 79
    Var = 80


