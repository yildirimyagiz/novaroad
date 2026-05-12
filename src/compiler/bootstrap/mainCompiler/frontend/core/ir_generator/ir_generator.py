#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class IRBasicBlock:
    pass

@dataclass
class IRFunction:
    pass

@dataclass
class IRGenerator:
    pass

@dataclass
class IRGlobal:
    pass

@dataclass
class IRModule:
    pass

@dataclass
class IRParameter:
    pass

@dataclass
class IRValue:
    pass

class IRInstruction(IntEnum):
    """IRInstruction definition"""
    Const = 0
    Add = 1
    Sub = 2
    Mul = 3
    Div = 4
    Rem = 5
    Pow = 6
    Eq = 7
    Ne = 8
    Lt = 9
    Gt = 10
    Le = 11
    Ge = 12
    And = 13
    Or = 14
    BitAnd = 15
    BitOr = 16
    BitXor = 17
    Shl = 18
    Shr = 19
    Load = 20
    Store = 21
    Alloca = 22
    GetElementPtr = 23
    Phi = 24
    Call = 25
    Bitcast = 26
    Trunc = 27
    Sext = 28
    Zext = 29
    TensorOp = 30
    FlowOp = 31
    UnitOp = 32
    Sync = 33
    Launch = 34
    Grad = 35
    Backprop = 36

class SyncKind(IntEnum):
    """SyncKind definition"""
    Threads = 37
    Memory = 38
    Device = 39

class IRTerminator(IntEnum):
    """IRTerminator definition"""
    Ret = 40
    Br = 41
    CondBr = 42
    Switch = 43
    Unreachable = 44

class IRType(IntEnum):
    """IRType definition"""
    Void = 45
    Bool = 46
    Int8 = 47
    Int16 = 48
    Int32 = 49
    Int64 = 50
    Float32 = 51
    Float64 = 52
    Pointer = 53
    Array = 54
    Struct = 55
    Function = 56
    Tensor = 57
    Flow = 58
    Quantity = 59

class IRConstant(IntEnum):
    """IRConstant definition"""
    Bool = 60
    Int = 61
    Float = 62
    Str = 63
    Array = 64
    Struct = 65
    Null = 66

class TensorOperation(IntEnum):
    """TensorOperation definition"""
    MatMul = 67
    Transpose = 68
    Reshape = 69
    Slice = 70
    Concat = 71
    Reduce = 72

class FlowOperation(IntEnum):
    """FlowOperation definition"""
    SignalCreate = 73
    StreamCreate = 74
    TaskSpawn = 75
    ChannelSend = 76
    ChannelRecv = 77

class UnitOperation(IntEnum):
    """UnitOperation definition"""
    Convert = 78
    Add = 79
    Mul = 80

class IRError(IntEnum):
    """IRError definition"""
    UndefinedSymbol = 81
    TypeMismatch = 82
    UnsupportedOperation = 83
    InvalidAST = 84


