#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class WASMCodeGenerator:
    pass

@dataclass
class WASMDataSegment:
    pass

@dataclass
class WASMExport:
    pass

@dataclass
class WASMFunction:
    pass

@dataclass
class WASMFunctionRef:
    pass

@dataclass
class WASMFunctionTranslator:
    pass

@dataclass
class WASMFunctionType:
    pass

@dataclass
class WASMGlobal:
    pass

@dataclass
class WASMGlobalRef:
    pass

@dataclass
class WASMGlobalType:
    pass

@dataclass
class WASMImport:
    pass

@dataclass
class WASMInstance:
    pass

@dataclass
class WASMLimits:
    pass

@dataclass
class WASMMemory:
    pass

@dataclass
class WASMMemoryConfig:
    pass

@dataclass
class WASMMemoryRef:
    pass

@dataclass
class WASMMemoryType:
    pass

@dataclass
class WASMModule:
    pass

@dataclass
class WASMModuleBuilder:
    pass

@dataclass
class WASMRuntime:
    pass

@dataclass
class WASMTableType:
    pass

@dataclass
class WASMTarget:
    pass

@dataclass
class WASMType:
    pass

@dataclass
class WASMTypeMapper:
    pass

class WASMOptimizationLevel(IntEnum):
    """WASMOptimizationLevel definition"""
    Debug = 0
    ReleaseSmall = 1
    ReleaseFast = 2

class WASMInstruction(IntEnum):
    """WASMInstruction definition"""
    Unreachable = 3
    Nop = 4
    Block = 5
    Loop = 6
    If = 7
    Br = 8
    BrIf = 9
    BrTable = 10
    Return = 11
    Call = 12
    CallIndirect = 13
    LocalGet = 14
    LocalSet = 15
    LocalTee = 16
    GlobalGet = 17
    GlobalSet = 18
    I32Load = 19
    I64Load = 20
    F32Load = 21
    F64Load = 22
    I32Store = 23
    I64Store = 24
    F32Store = 25
    F64Store = 26
    MemorySize = 27
    MemoryGrow = 28
    I32Const = 29
    I64Const = 30
    F32Const = 31
    F64Const = 32
    I32Add = 33
    I32Sub = 34
    I32Mul = 35
    I32DivS = 36
    I32DivU = 37
    I32RemS = 38
    I32RemU = 39
    I64Add = 40
    I64Sub = 41
    I64Mul = 42
    I64DivS = 43
    I64DivU = 44
    I64RemS = 45
    I64RemU = 46
    F32Add = 47
    F32Sub = 48
    F32Mul = 49
    F32Div = 50
    F64Add = 51
    F64Sub = 52
    F64Mul = 53
    F64Div = 54
    I32Eq = 55
    I32Ne = 56
    I32LtS = 57
    I32LtU = 58
    I32GtS = 59
    I32GtU = 60
    I32LeS = 61
    I32LeU = 62
    I32GeS = 63
    I32GeU = 64
    I64Eq = 65
    I64Ne = 66
    I64LtS = 67
    I64LtU = 68
    I64GtS = 69
    I64GtU = 70
    I64LeS = 71
    I64LeU = 72
    I64GeS = 73
    I64GeU = 74
    F32Eq = 75
    F32Ne = 76
    F32Lt = 77
    F32Gt = 78
    F32Le = 79
    F32Ge = 80
    F64Eq = 81
    F64Ne = 82
    F64Lt = 83
    F64Gt = 84
    F64Le = 85
    F64Ge = 86

class WASMValueType(IntEnum):
    """WASMValueType definition"""
    I32 = 87
    I64 = 88
    F32 = 89
    F64 = 90

class WASMExportKind(IntEnum):
    """WASMExportKind definition"""
    Function = 91
    Table = 92
    Memory = 93
    Global = 94

class WASMImportKind(IntEnum):
    """WASMImportKind definition"""
    Function = 95
    Table = 96
    Memory = 97
    Global = 98

class WASMElementType(IntEnum):
    """WASMElementType definition"""
    FuncRef = 99

class WASMExportValue(IntEnum):
    """WASMExportValue definition"""
    Function = 100
    Memory = 101
    Global = 102

class WASMValue(IntEnum):
    """WASMValue definition"""
    I32 = 103
    I64 = 104
    F32 = 105
    F64 = 106

class WASMError(IntEnum):
    """WASMError definition"""
    CompilationFailed = 107
    TypeMappingError = 108
    CodeGenerationError = 109
    ValidationError = 110


