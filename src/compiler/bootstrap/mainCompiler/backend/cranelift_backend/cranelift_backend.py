#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class BasicBlock:
    pass

@dataclass
class CodegenContext:
    pass

@dataclass
class CompiledFunction:
    pass

@dataclass
class CompiledModule:
    pass

@dataclass
class CraneliftBackend:
    pass

@dataclass
class CraneliftFunction:
    pass

@dataclass
class CraneliftGlobal:
    pass

@dataclass
class CraneliftModule:
    pass

@dataclass
class DataSection:
    pass

@dataclass
class DebugInfo:
    pass

@dataclass
class FunctionBody:
    pass

@dataclass
class FunctionSignature:
    pass

@dataclass
class LineEntry:
    pass

@dataclass
class LocalVar:
    pass

@dataclass
class Relocation:
    pass

@dataclass
class SymbolInfo:
    pass

@dataclass
class TargetConfig:
    pass

@dataclass
class Value:
    pass

class OptLevel(IntEnum):
    """OptLevel definition"""
    None = 0
    Speed = 1
    SpeedAndSize = 2

class Endianness(IntEnum):
    """Endianness definition"""
    Little = 3
    Big = 4

class CallingConvention(IntEnum):
    """CallingConvention definition"""
    SystemV = 5
    WindowsFastcall = 6
    Wasm = 7

class Instruction(IntEnum):
    """Instruction definition"""
    Jump = 8
    Branch = 9
    Return = 10
    Call = 11
    Load = 12
    Store = 13
    GlobalLoad = 14
    GlobalStore = 15
    IAdd = 16
    ISub = 17
    IMul = 18
    IDiv = 19
    FAdd = 20
    FSub = 21
    FMul = 22
    FDiv = 23
    ICmp = 24
    FCmp = 25
    SExt = 26
    ZExt = 27
    Trunc = 28
    F32ToF64 = 29
    F64ToF32 = 30
    I32ToF64 = 31
    F64ToI32 = 32
    Phi = 33

class CmpOp(IntEnum):
    """CmpOp definition"""
    Eq = 34
    Ne = 35
    Lt = 36
    Le = 37
    Gt = 38
    Ge = 39

class CraneliftType(IntEnum):
    """CraneliftType definition"""
    I8 = 40
    I16 = 41
    I32 = 42
    I64 = 43
    U8 = 44
    U16 = 45
    U32 = 46
    U64 = 47
    F32 = 48
    F64 = 49
    B1 = 50
    Ptr = 51
    Array = 52
    Struct = 53
    Void = 54

class RelocationKind(IntEnum):
    """RelocationKind definition"""
    Absolute32 = 55
    Relative32 = 56
    FunctionCall = 57
    DataLoad = 58

class SymbolKind(IntEnum):
    """SymbolKind definition"""
    Function = 59
    Global = 60
    Local = 61

class SectionKind(IntEnum):
    """SectionKind definition"""
    ReadOnlyData = 62
    ReadWriteData = 63
    UninitializedData = 64

class CodegenError(IntEnum):
    """CodegenError definition"""
    UnsupportedInstruction = 65
    TypeMismatch = 66
    TargetNotSupported = 67
    CompilationFailed = 68


