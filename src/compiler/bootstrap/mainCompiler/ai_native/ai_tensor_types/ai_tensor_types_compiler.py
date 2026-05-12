#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("TensorStructure", ['Image { channels: u8, format: PixelFormat }', 'Sequence { length: usize, padding: PaddingType }', 'Graph { nodes: usize, edges: usize, directed: bool }', 'Sparse { format: SparseFormat, density: f64 }', 'Quantized { scale: T, zero_point: T }'], "TensorStructure definition"),
    ("PixelFormat", ['RGB', 'RGBA', 'BGR', 'BGRA', 'Gray', 'HSV', 'YUV'], "PixelFormat definition"),
    ("SparseFormat", ['COO', 'CSR', 'CSC', 'BlockSparse'], "SparseFormat definition"),
    ("PaddingType", ['Zero', 'Reflect', 'Replicate', 'Edge'], "PaddingType definition"),
    ("PrivacyLevel", ['Public', 'DifferentialPrivacy(epsilon: f64)', 'SecureAggregation', 'HomomorphicEncryption'], "PrivacyLevel definition"),
    ("EncryptionScheme", ['None', 'AES256', 'RSA2048', 'LatticeBased', 'Homomorphic'], "EncryptionScheme definition"),
    ("AggregationMethod", ['FedAvg', 'FedProx', 'Scaffold', 'Mime'], "AggregationMethod definition"),
]

@dataclass
class AiTensorTypesCompilerGenerator:
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
        output += "\n@dataclass\nclass AdaptiveTensor:\n    pass\n\n@dataclass\nclass FederatedTensor:\n    pass\n\n@dataclass\nclass MLPredictor:\n    pass\n\n@dataclass\nclass OptimizationRecord:\n    pass\n\n@dataclass\nclass QuantumTensor:\n    pass\n\n@dataclass\nclass StructuredTensor:\n    pass\n\n@dataclass\nclass TensorOptimizer:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_AI_TENSOR_TYPES_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_AI_TENSOR_TYPES_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = AiTensorTypesCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
