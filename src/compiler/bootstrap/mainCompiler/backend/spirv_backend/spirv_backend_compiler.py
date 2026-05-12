#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("GpuTarget", ['Vulkan', 'OpenCL', 'Metal', 'DirectX'], "GpuTarget definition"),
    ("Capability", ['Shader', 'Kernel', 'Int64', 'Float64', 'AtomicStorage', 'StorageImageReadWithoutFormat', 'StorageImageWriteWithoutFormat', 'VariablePointers', 'VariablePointersStorageBuffer'], "Capability definition"),
    ("MemoryModel", ['Simple', 'GLSL450', 'OpenCL', 'Vulkan'], "MemoryModel definition"),
    ("ExecutionModel", ['Vertex', 'TessellationControl', 'TessellationEvaluation', 'Geometry', 'Fragment', 'GLCompute', 'Kernel'], "ExecutionModel definition"),
    ("Operand", ['Id(u32)', 'LiteralInt(u32)', 'LiteralString(String)', 'LiteralFloat(f32)', 'LiteralComposite(Vec<Operand>)'], "Operand definition"),
    ("DebugOp", ['Source', 'SourceExtension', 'Name', 'MemberName', 'String', 'Line'], "DebugOp definition"),
    ("AnnotationOp", ['Decorate', 'MemberDecorate', 'GroupDecorate', 'GroupMemberDecorate'], "AnnotationOp definition"),
    ("TypeOp", ['Void', 'Bool', 'Int', 'Float', 'Vector', 'Matrix', 'Image', 'Sampler', 'SampledImage', 'Array', 'RuntimeArray', 'Struct', 'Pointer', 'Function', 'Event', 'DeviceEvent', 'ReserveId', 'Queue', 'Pipe', 'ForwardPointer'], "TypeOp definition"),
    ("ConstantOp", ['True', 'False', 'Constant', 'ConstantComposite', 'ConstantSampler', 'ConstantNull', 'SpecConstant', 'SpecConstantComposite', 'SpecConstantOp'], "ConstantOp definition"),
    ("StorageClass", ['UniformConstant', 'Input', 'Uniform', 'Output', 'Workgroup', 'CrossWorkgroup', 'Private', 'Function', 'Generic', 'PushConstant', 'AtomicCounter', 'Image', 'StorageBuffer'], "StorageClass definition"),
    ("FunctionControl", ['None', 'Inline', 'DontInline', 'Pure', 'Const'], "FunctionControl definition"),
    ("ExecutionModeKind", ['Invocations', 'SpacingEqual', 'SpacingFractionalEven', 'SpacingFractionalOdd', 'VertexOrderCw', 'VertexOrderCcw', 'PixelCenterInteger', 'OriginUpperLeft', 'OriginLowerLeft', 'EarlyFragmentTests', 'PointMode', 'Xfb', 'DepthReplacing', 'DepthGreater', 'DepthLess', 'DepthUnchanged', 'LocalSize', 'LocalSizeHint', 'InputPoints', 'InputLines', 'InputLinesAdjacency', 'Triangles', 'InputTrianglesAdjacency', 'Quads', 'Isolines', 'OutputVertices', 'OutputPoints', 'OutputLineStrip', 'OutputTriangleStrip', 'VecTypeHint', 'ContractionOff', 'Initializer', 'Finalizer', 'SubgroupSize', 'SubgroupsPerWorkgroup', 'SubgroupsPerWorkgroupId', 'LocalSizeId', 'LocalSizeHintId', 'PostDepthCoverage', 'DenormPreserve', 'DenormFlushToZero', 'SignedZeroInfNanPreserve', 'RoundingModeRTE', 'RoundingModeRTZ', 'StencilRefReplacingEXT', 'OutputLinesNV', 'OutputPrimitivesNV', 'DerivativeGroupQuadsNV', 'DerivativeGroupLinearNV', 'OutputTrianglesNV', 'PixelInterlockOrderedEXT', 'PixelInterlockUnorderedEXT', 'SampleInterlockOrderedEXT', 'SampleInterlockUnorderedEXT', 'ShadingRateInterlockOrderedEXT', 'ShadingRateInterlockUnorderedEXT', 'SharedLocalMemorySizeINTEL', 'RoundingModeRTPINTEL', 'RoundingModeRTNINTEL', 'FloatingPointModeALTINTEL', 'FloatingPointModeIEEEINTEL', 'MaxWorkgroupSizeINTEL', 'MaxWorkDimINTEL', 'NoGlobalOffsetINTEL', 'NumSIMDWorkitemsINTEL'], "ExecutionModeKind definition"),
    ("CodegenError", ['UnsupportedInstruction', 'UnsupportedType', 'UnsupportedConstant', 'TypeNotFound(String)', 'CompilationFailed(String)'], "CodegenError definition"),
    ("ValidationError", ['InvalidHeader', 'MissingCapability(Capability)', 'TypeMismatch', 'InvalidInstruction'], "ValidationError definition"),
]

@dataclass
class SpirvBackendCompilerGenerator:
    """Python Enum ve C Tag üretici."""
    cases: List[Tuple[str, List[str], str]] = field(default_factory=lambda: CASES)
    tag_map: Dict[str, int] = field(default_factory=dict)

    def compile(self):
        offset = 0
        for group_name, variants, _ in self.cases:
            for i, var in enumerate(variants):
                self.tag_map[f"{group_name}::{var}"] = offset + i
            offset += len(variants)
        return self

    def emit_python_enums(self) -> str:
        output = "#!/usr/bin/env python3\nfrom enum import IntEnum\nfrom dataclasses import dataclass, field\nfrom typing import List, Optional, Any\n\n"
        output += "\n@dataclass\nclass Annotation:\n    pass\n\n@dataclass\nclass Block:\n    pass\n\n@dataclass\nclass CodegenContext:\n    pass\n\n@dataclass\nclass ConstantInstruction:\n    pass\n\n@dataclass\nclass DebugInstruction:\n    pass\n\n@dataclass\nclass EntryPoint:\n    pass\n\n@dataclass\nclass ExecutionMode:\n    pass\n\n@dataclass\nclass Function:\n    pass\n\n@dataclass\nclass Instruction:\n    pass\n\n@dataclass\nclass ModuleHeader:\n    pass\n\n@dataclass\nclass Parameter:\n    pass\n\n@dataclass\nclass SpirvBackend:\n    pass\n\n@dataclass\nclass SpirvModule:\n    pass\n\n@dataclass\nclass SpirvVersion:\n    pass\n\n@dataclass\nclass TypeInstruction:\n    pass\n\n@dataclass\nclass VariableInstruction:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_SPIRV_BACKEND_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_SPIRV_BACKEND_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = SpirvBackendCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
