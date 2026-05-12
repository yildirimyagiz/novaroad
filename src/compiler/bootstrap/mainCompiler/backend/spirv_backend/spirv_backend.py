#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Annotation:
    pass

@dataclass
class Block:
    pass

@dataclass
class CodegenContext:
    pass

@dataclass
class ConstantInstruction:
    pass

@dataclass
class DebugInstruction:
    pass

@dataclass
class EntryPoint:
    pass

@dataclass
class ExecutionMode:
    pass

@dataclass
class Function:
    pass

@dataclass
class Instruction:
    pass

@dataclass
class ModuleHeader:
    pass

@dataclass
class Parameter:
    pass

@dataclass
class SpirvBackend:
    pass

@dataclass
class SpirvModule:
    pass

@dataclass
class SpirvVersion:
    pass

@dataclass
class TypeInstruction:
    pass

@dataclass
class VariableInstruction:
    pass

class GpuTarget(IntEnum):
    """GpuTarget definition"""
    Vulkan = 0
    OpenCL = 1
    Metal = 2
    DirectX = 3

class Capability(IntEnum):
    """Capability definition"""
    Shader = 4
    Kernel = 5
    Int64 = 6
    Float64 = 7
    AtomicStorage = 8
    StorageImageReadWithoutFormat = 9
    StorageImageWriteWithoutFormat = 10
    VariablePointers = 11
    VariablePointersStorageBuffer = 12

class MemoryModel(IntEnum):
    """MemoryModel definition"""
    Simple = 13
    GLSL450 = 14
    OpenCL = 15
    Vulkan = 16

class ExecutionModel(IntEnum):
    """ExecutionModel definition"""
    Vertex = 17
    TessellationControl = 18
    TessellationEvaluation = 19
    Geometry = 20
    Fragment = 21
    GLCompute = 22
    Kernel = 23

class Operand(IntEnum):
    """Operand definition"""
    Id = 24
    LiteralInt = 25
    LiteralString = 26
    LiteralFloat = 27
    LiteralComposite = 28

class DebugOp(IntEnum):
    """DebugOp definition"""
    Source = 29
    SourceExtension = 30
    Name = 31
    MemberName = 32
    String = 33
    Line = 34

class AnnotationOp(IntEnum):
    """AnnotationOp definition"""
    Decorate = 35
    MemberDecorate = 36
    GroupDecorate = 37
    GroupMemberDecorate = 38

class TypeOp(IntEnum):
    """TypeOp definition"""
    Void = 39
    Bool = 40
    Int = 41
    Float = 42
    Vector = 43
    Matrix = 44
    Image = 45
    Sampler = 46
    SampledImage = 47
    Array = 48
    RuntimeArray = 49
    Struct = 50
    Pointer = 51
    Function = 52
    Event = 53
    DeviceEvent = 54
    ReserveId = 55
    Queue = 56
    Pipe = 57
    ForwardPointer = 58

class ConstantOp(IntEnum):
    """ConstantOp definition"""
    True = 59
    False = 60
    Constant = 61
    ConstantComposite = 62
    ConstantSampler = 63
    ConstantNull = 64
    SpecConstant = 65
    SpecConstantComposite = 66
    SpecConstantOp = 67

class StorageClass(IntEnum):
    """StorageClass definition"""
    UniformConstant = 68
    Input = 69
    Uniform = 70
    Output = 71
    Workgroup = 72
    CrossWorkgroup = 73
    Private = 74
    Function = 75
    Generic = 76
    PushConstant = 77
    AtomicCounter = 78
    Image = 79
    StorageBuffer = 80

class FunctionControl(IntEnum):
    """FunctionControl definition"""
    None = 81
    Inline = 82
    DontInline = 83
    Pure = 84
    Const = 85

class ExecutionModeKind(IntEnum):
    """ExecutionModeKind definition"""
    Invocations = 86
    SpacingEqual = 87
    SpacingFractionalEven = 88
    SpacingFractionalOdd = 89
    VertexOrderCw = 90
    VertexOrderCcw = 91
    PixelCenterInteger = 92
    OriginUpperLeft = 93
    OriginLowerLeft = 94
    EarlyFragmentTests = 95
    PointMode = 96
    Xfb = 97
    DepthReplacing = 98
    DepthGreater = 99
    DepthLess = 100
    DepthUnchanged = 101
    LocalSize = 102
    LocalSizeHint = 103
    InputPoints = 104
    InputLines = 105
    InputLinesAdjacency = 106
    Triangles = 107
    InputTrianglesAdjacency = 108
    Quads = 109
    Isolines = 110
    OutputVertices = 111
    OutputPoints = 112
    OutputLineStrip = 113
    OutputTriangleStrip = 114
    VecTypeHint = 115
    ContractionOff = 116
    Initializer = 117
    Finalizer = 118
    SubgroupSize = 119
    SubgroupsPerWorkgroup = 120
    SubgroupsPerWorkgroupId = 121
    LocalSizeId = 122
    LocalSizeHintId = 123
    PostDepthCoverage = 124
    DenormPreserve = 125
    DenormFlushToZero = 126
    SignedZeroInfNanPreserve = 127
    RoundingModeRTE = 128
    RoundingModeRTZ = 129
    StencilRefReplacingEXT = 130
    OutputLinesNV = 131
    OutputPrimitivesNV = 132
    DerivativeGroupQuadsNV = 133
    DerivativeGroupLinearNV = 134
    OutputTrianglesNV = 135
    PixelInterlockOrderedEXT = 136
    PixelInterlockUnorderedEXT = 137
    SampleInterlockOrderedEXT = 138
    SampleInterlockUnorderedEXT = 139
    ShadingRateInterlockOrderedEXT = 140
    ShadingRateInterlockUnorderedEXT = 141
    SharedLocalMemorySizeINTEL = 142
    RoundingModeRTPINTEL = 143
    RoundingModeRTNINTEL = 144
    FloatingPointModeALTINTEL = 145
    FloatingPointModeIEEEINTEL = 146
    MaxWorkgroupSizeINTEL = 147
    MaxWorkDimINTEL = 148
    NoGlobalOffsetINTEL = 149
    NumSIMDWorkitemsINTEL = 150

class CodegenError(IntEnum):
    """CodegenError definition"""
    UnsupportedInstruction = 151
    UnsupportedType = 152
    UnsupportedConstant = 153
    TypeNotFound = 154
    CompilationFailed = 155

class ValidationError(IntEnum):
    """ValidationError definition"""
    InvalidHeader = 156
    MissingCapability = 157
    TypeMismatch = 158
    InvalidInstruction = 159


