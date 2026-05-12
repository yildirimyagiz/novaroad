#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class BlockType:
    pass

@dataclass
class CodegenContext:
    pass

@dataclass
class CustomSection:
    pass

@dataclass
class DataSegment:
    pass

@dataclass
class ElementSegment:
    pass

@dataclass
class Expression:
    pass

@dataclass
class FunctionBody:
    pass

@dataclass
class FunctionType:
    pass

@dataclass
class Limits:
    pass

@dataclass
class MemArg:
    pass

@dataclass
class MemoryConfig:
    pass

@dataclass
class MemoryType:
    pass

@dataclass
class TableType:
    pass

@dataclass
class WasmBackend:
    pass

@dataclass
class WasmExport:
    pass

@dataclass
class WasmFunction:
    pass

@dataclass
class WasmGlobal:
    pass

@dataclass
class WasmGlobalType:
    pass

@dataclass
class WasmImport:
    pass

@dataclass
class WasmModule:
    pass

@dataclass
class WasmVersion:
    pass

class WasmTarget(IntEnum):
    """WasmTarget definition"""
    Browser = 0
    NodeJS = 1
    WASI = 2
    Emscripten = 3

class WasmFeature(IntEnum):
    """WasmFeature definition"""
    MutableGlobals = 4
    NontrappingFPToInt = 5
    SignExtensionOps = 6
    MultiValue = 7
    BulkMemory = 8
    ReferenceTypes = 9
    SIMD128 = 10
    RelaxedSIMD = 11
    Threads = 12
    ExceptionHandling = 13
    Memory64 = 14
    ExtendedConst = 15

class WasmType(IntEnum):
    """WasmType definition"""
    I32 = 16
    I64 = 17
    F32 = 18
    F64 = 19
    V128 = 20
    FuncRef = 21
    ExternRef = 22

class ImportKind(IntEnum):
    """ImportKind definition"""
    Function = 23
    Table = 24
    Memory = 25
    Global = 26

class ExportKind(IntEnum):
    """ExportKind definition"""
    Function = 27
    Table = 28
    Memory = 29
    Global = 30

class Instruction(IntEnum):
    """Instruction definition"""
    Unreachable = 31
    Nop = 32
    Block = 33
    Loop = 34
    If = 35
    Br = 36
    BrIf = 37
    BrTable = 38
    Return = 39
    Call = 40
    CallIndirect = 41
    Drop = 42
    Select = 43
    LocalGet = 44
    LocalSet = 45
    LocalTee = 46
    GlobalGet = 47
    GlobalSet = 48
    I32Load = 49
    I64Load = 50
    F32Load = 51
    F64Load = 52
    I32Load8S = 53
    I32Load8U = 54
    I32Load16S = 55
    I32Load16U = 56
    I64Load8S = 57
    I64Load8U = 58
    I64Load16S = 59
    I64Load16U = 60
    I64Load32S = 61
    I64Load32U = 62
    I32Store = 63
    I64Store = 64
    F32Store = 65
    F64Store = 66
    I32Store8 = 67
    I32Store16 = 68
    I64Store8 = 69
    I64Store16 = 70
    I64Store32 = 71
    MemorySize = 72
    MemoryGrow = 73
    I32Const = 74
    I64Const = 75
    F32Const = 76
    F64Const = 77
    I32Eqz = 78
    I32Eq = 79
    I32Ne = 80
    I32LtS = 81
    I32LtU = 82
    I32GtS = 83
    I32GtU = 84
    I32LeS = 85
    I32LeU = 86
    I32GeS = 87
    I32GeU = 88
    I32Clz = 89
    I32Ctz = 90
    I32Popcnt = 91
    I32Add = 92
    I32Sub = 93
    I32Mul = 94
    I32DivS = 95
    I32DivU = 96
    I32RemS = 97
    I32RemU = 98
    I32And = 99
    I32Or = 100
    I32Xor = 101
    I32Shl = 102
    I32ShrS = 103
    I32ShrU = 104
    I32Rotl = 105
    I32Rotr = 106
    I64Eqz = 107
    I64Eq = 108
    I64Ne = 109
    I64LtS = 110
    I64LtU = 111
    I64GtS = 112
    I64GtU = 113
    I64LeS = 114
    I64LeU = 115
    I64GeS = 116
    I64GeU = 117
    I64Clz = 118
    I64Ctz = 119
    I64Popcnt = 120
    I64Add = 121
    I64Sub = 122
    I64Mul = 123
    I64DivS = 124
    I64DivU = 125
    I64RemS = 126
    I64RemU = 127
    I64And = 128
    I64Or = 129
    I64Xor = 130
    I64Shl = 131
    I64ShrS = 132
    I64ShrU = 133
    I64Rotl = 134
    I64Rotr = 135
    F32Eq = 136
    F32Ne = 137
    F32Lt = 138
    F32Gt = 139
    F32Le = 140
    F32Ge = 141
    F32Abs = 142
    F32Neg = 143
    F32Ceil = 144
    F32Floor = 145
    F32Trunc = 146
    F32Nearest = 147
    F32Sqrt = 148
    F32Add = 149
    F32Sub = 150
    F32Mul = 151
    F32Div = 152
    F32Min = 153
    F32Max = 154
    F32Copysign = 155
    F64Eq = 156
    F64Ne = 157
    F64Lt = 158
    F64Gt = 159
    F64Le = 160
    F64Ge = 161
    F64Abs = 162
    F64Neg = 163
    F64Ceil = 164
    F64Floor = 165
    F64Trunc = 166
    F64Nearest = 167
    F64Sqrt = 168
    F64Add = 169
    F64Sub = 170
    F64Mul = 171
    F64Div = 172
    F64Min = 173
    F64Max = 174
    F64Copysign = 175
    I32WrapI64 = 176
    I32TruncF32S = 177
    I32TruncF32U = 178
    I32TruncF64S = 179
    I32TruncF64U = 180
    I64ExtendI32S = 181
    I64ExtendI32U = 182
    I64TruncF32S = 183
    I64TruncF32U = 184
    I64TruncF64S = 185
    I64TruncF64U = 186
    F32ConvertI32S = 187
    F32ConvertI32U = 188
    F32ConvertI64S = 189
    F32ConvertI64U = 190
    F32DemoteF64 = 191
    F64ConvertI32S = 192
    F64ConvertI32U = 193
    F64ConvertI64S = 194
    F64ConvertI64U = 195
    F64PromoteF32 = 196
    I32ReinterpretF32 = 197
    I64ReinterpretF64 = 198
    F32ReinterpretI32 = 199
    F64ReinterpretI64 = 200
    V128Load = 201
    V128Store = 202
    V128Const = 203
    I8x16Shuffle = 204
    MemoryInit = 205
    DataDrop = 206
    MemoryCopy = 207
    MemoryFill = 208
    RefNull = 209
    RefIsNull = 210
    RefFunc = 211
    TableGet = 212
    TableSet = 213
    TableSize = 214
    TableGrow = 215
    TableFill = 216
    TableCopy = 217
    TableInit = 218
    ElemDrop = 219

class CodegenError(IntEnum):
    """CodegenError definition"""
    UnsupportedInstruction = 220
    UnsupportedType = 221
    FunctionNotFound = 222
    CompilationFailed = 223

class ValidationError(IntEnum):
    """ValidationError definition"""
    UnsupportedVersion = 224
    InvalidTypeIndex = 225
    InvalidFunctionIndex = 226
    MalformedInstruction = 227


