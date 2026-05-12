#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("GPUPlatform", ['Metal', 'Vulkan', 'DirectX12', 'CUDA', 'OpenCL'], "GPUPlatform definition"),
    ("QueuePriority", ['Low', 'Normal', 'High', 'RealTime'], "QueuePriority definition"),
    ("BufferUsage", ['Vertex', 'Index', 'Uniform', 'Storage', 'Texture'], "BufferUsage definition"),
    ("ShaderType", ['Vertex', 'Fragment', 'Compute', 'Geometry', 'TessellationControl', 'TessellationEvaluation'], "ShaderType definition"),
    ("CullMode", ['None', 'Front', 'Back', 'FrontAndBack'], "CullMode definition"),
    ("PolygonMode", ['Fill', 'Line', 'Point'], "PolygonMode definition"),
    ("GPUCommand", ['Draw(DrawCommand)', 'Compute(ComputeCommand)', 'Copy(CopyCommand)', 'Barrier(BarrierCommand)'], "GPUCommand definition"),
    ("PipelineStage", ['TopOfPipe', 'DrawIndirect', 'VertexInput', 'VertexShader', 'FragmentShader', 'ComputeShader', 'Transfer', 'BottomOfPipe'], "PipelineStage definition"),
    ("PerformanceRecommendation", ['IncreaseDrawCalls', 'ReduceDrawCalls', 'UseLOD', 'ReduceTextureQuality', 'ImplementTextureStreaming', 'UseInstancing', 'OptimizeShaders', 'ReduceOverdraw'], "PerformanceRecommendation definition"),
    ("GPUError", ['OutOfMemory', 'AllocationFailed', 'CompilationFailed', 'PipelineCreationFailed', 'InvalidQueue', 'CommandSubmissionFailed', 'UnsupportedFeature'], "GPUError definition"),
    ("MemoryPoolType", ['General', 'GPU', 'Texture', 'Audio', 'Network'], "MemoryPoolType definition"),
    ("MemoryError", ['OutOfMemory', 'InvalidPool', 'InvalidBlock', 'AlignmentError', 'DoubleFree'], "MemoryError definition"),
    ("BottleneckType", ['CPU', 'GPU', 'Memory', 'DiskIO', 'Network', 'Battery'], "BottleneckType definition"),
    ("OptimizationType", ['GPUAcceleration', 'MemoryPooling', 'TextureCompression', 'ShaderOptimization', 'DrawCallBatching', 'LevelOfDetail', 'FrustumCulling', 'OcclusionCulling', 'Multithreading'], "OptimizationType definition"),
    ("QualityPreset", ['Low', 'Medium', 'High', 'Ultra'], "QualityPreset definition"),
]

@dataclass
class PerformanceOptimizationCompilerGenerator:
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
        output += "\n@dataclass\nclass AccessFlags:\n    pass\n\n@dataclass\nclass AdvancedMemoryManager:\n    pass\n\n@dataclass\nclass AllocationRecord:\n    pass\n\n@dataclass\nclass BarrierCommand:\n    pass\n\n@dataclass\nclass CPUProfiler:\n    pass\n\n@dataclass\nclass CacheEntry:\n    pass\n\n@dataclass\nclass ComputeCommand:\n    pass\n\n@dataclass\nclass CopyCommand:\n    pass\n\n@dataclass\nclass DrawCommand:\n    pass\n\n@dataclass\nclass FrameProfiler:\n    pass\n\n@dataclass\nclass GCObject:\n    pass\n\n@dataclass\nclass GPUAccelerator:\n    pass\n\n@dataclass\nclass GPUBuffer:\n    pass\n\n@dataclass\nclass GPUCommandQueue:\n    pass\n\n@dataclass\nclass GPUDeviceInfo:\n    pass\n\n@dataclass\nclass GPUDeviceInfoRaw:\n    pass\n\n@dataclass\nclass GPUMemoryPool:\n    pass\n\n@dataclass\nclass GPUPerformanceMetrics:\n    pass\n\n@dataclass\nclass GPUPerformanceMetricsRaw:\n    pass\n\n@dataclass\nclass GPUPipeline:\n    pass\n\n@dataclass\nclass GPUProfiler:\n    pass\n\n@dataclass\nclass GPUShader:\n    pass\n\n@dataclass\nclass GarbageCollector:\n    pass\n\n@dataclass\nclass LeakDetector:\n    pass\n\n@dataclass\nclass MemoryAllocator:\n    pass\n\n@dataclass\nclass MemoryBarrier:\n    pass\n\n@dataclass\nclass MemoryBlock:\n    pass\n\n@dataclass\nclass MemoryCache:\n    pass\n\n@dataclass\nclass MemoryPool:\n    pass\n\n@dataclass\nclass MemoryProfiler:\n    pass\n\n@dataclass\nclass MemorySnapshot:\n    pass\n\n@dataclass\nclass MemoryStats:\n    pass\n\n@dataclass\nclass NetworkProfiler:\n    pass\n\n@dataclass\nclass OptimizationEngine:\n    pass\n\n@dataclass\nclass PerformanceOptimizer:\n    pass\n\n@dataclass\nclass PerformanceProfiler:\n    pass\n\n@dataclass\nclass PerformanceRecommendations:\n    pass\n\n@dataclass\nclass PerformanceReport:\n    pass\n\n@dataclass\nclass PerformanceTargets:\n    pass\n\n@dataclass\nclass PipelineConfig:\n    pass\n\n@dataclass\nclass ShaderCache:\n    pass\n\n@dataclass\nclass ShaderStatistics:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_PERFORMANCE_OPTIMIZATION_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_PERFORMANCE_OPTIMIZATION_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = PerformanceOptimizationCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
