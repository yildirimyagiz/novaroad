#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("ViewType", ['Text', 'Button', 'Image', 'Container', 'List', 'Scroll', 'Stack', 'Custom(String)'], "ViewType definition"),
    ("ViewContent", ['Empty', 'Text(String)', 'Image(ImageSource)', 'Children(Vec<AuroraView>)'], "ViewContent definition"),
    ("BorderStyle", ['Solid', 'Dashed', 'Dotted'], "BorderStyle definition"),
    ("Alignment", ['TopLeading', 'Top', 'TopTrailing', 'Leading', 'Center', 'Trailing', 'BottomLeading', 'Bottom', 'BottomTrailing'], "Alignment definition"),
    ("GestureRecognizer", ['Tap(Box<dyn Fn()>)', 'Swipe(SwipeDirection, Box<dyn Fn()>)', 'Pinch(Box<dyn Fn(f64)>)', 'Rotate(Box<dyn Fn(f64)>)', 'Pan(Box<dyn Fn(Point)>)'], "GestureRecognizer definition"),
    ("SwipeDirection", ['Up', 'Down', 'Left', 'Right'], "SwipeDirection definition"),
    ("Gesture", ['Tap', 'Swipe(SwipeDirection)', 'Pinch { scale: f64 }', 'Rotate { angle: f64 }', 'Pan { translation: Point }'], "Gesture definition"),
    ("AnimationCurve", ['Linear', 'EaseIn', 'EaseOut', 'EaseInOut', 'Spring { damping: f64, velocity: f64 }', 'Custom(Box<dyn Fn(f64) -> f64>)'], "AnimationCurve definition"),
    ("AccessibilityTrait", ['Button', 'Header', 'Image', 'StaticText', 'Adjustable', 'Selected', 'NotEnabled', 'SummaryElement', 'KeyboardKey', 'SearchField', 'EditableText', 'AllowsDirectInteraction'], "AccessibilityTrait definition"),
]

@dataclass
class AuroraAdvancedUiCompilerGenerator:
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
        output += "\n@dataclass\nclass ActiveAnimation:\n    pass\n\n@dataclass\nclass Animation:\n    pass\n\n@dataclass\nclass Animator:\n    pass\n\n@dataclass\nclass AuroraAccessibilityInfo:\n    pass\n\n@dataclass\nclass AuroraView:\n    pass\n\n@dataclass\nclass BackgroundModifier:\n    pass\n\n@dataclass\nclass Border:\n    pass\n\n@dataclass\nclass Color:\n    pass\n\n@dataclass\nclass EdgeInsets:\n    pass\n\n@dataclass\nclass Environment:\n    pass\n\n@dataclass\nclass FrameModifier:\n    pass\n\n@dataclass\nclass ImageSource:\n    pass\n\n@dataclass\nclass Modifiers:\n    pass\n\n@dataclass\nclass Observable:\n    pass\n\n@dataclass\nclass PaddingModifier:\n    pass\n\n@dataclass\nclass Point:\n    pass\n\n@dataclass\nclass Rect:\n    pass\n\n@dataclass\nclass RenderContext:\n    pass\n\n@dataclass\nclass Shadow:\n    pass\n\n@dataclass\nclass ShadowModifier:\n    pass\n\n@dataclass\nclass Size:\n    pass\n\n@dataclass\nclass State:\n    pass\n\n@dataclass\nclass Transform:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_AURORA_ADVANCED_UI_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_AURORA_ADVANCED_UI_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = AuroraAdvancedUiCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
