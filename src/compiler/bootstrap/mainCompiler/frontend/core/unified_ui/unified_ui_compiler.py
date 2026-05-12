#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("Element", ['None', 'Text(String)', 'Node {\n        tag:      String,\n        attrs:    Vec<Attribute>,\n        children: Vec<Element>,\n        key:      Option<String>,\n        node_id:  Option<i64>,     \n    }', 'Component {\n        name:     String,\n        props:    HashMap<String, PropValue>,\n        children: Vec<Element>,\n    }', 'Fragment(Vec<Element>)', 'Portal {                        \n        target_id: i64,\n        content:   Box<Element>,\n    }'], "Element definition"),
    ("AttrValue", ['Text(String)', 'Number(f64)', 'Bool(bool)', 'Handler(EventHandler)', 'Style(StyleSheet)', 'None'], "AttrValue definition"),
    ("PropValue", ['Str(String)', 'Int(i64)', 'Float(f64)', 'Bool(bool)', 'List(Vec<PropValue>)', 'Element(Box<Element>)', 'None'], "PropValue definition"),
    ("PlatformEvent", ['Click    { x: f64, y: f64, target_id: i64, button: i64 }', 'DblClick { x: f64, y: f64, target_id: i64 }', 'Input    { target_id: i64, value: String }', 'Change   { target_id: i64, value: String, checked: bool }', 'KeyDown  { key: String, code: String, modifiers: ModifierKeys }', 'KeyUp    { key: String, code: String, modifiers: ModifierKeys }', 'Scroll   { delta_x: f64, delta_y: f64 }', 'Resize   { width: f64, height: f64 }', 'Focus    { target_id: i64 }', 'Blur     { target_id: i64 }', 'Touch    { phase: TouchPhase, x: f64, y: f64, id: i64 }', 'Custom   { name: String, data: HashMap<String, String> }'], "PlatformEvent definition"),
    ("TouchPhase", ['Began', 'Moved', 'Ended', 'Cancelled'], "TouchPhase definition"),
    ("Mutation", ['CreateElement     { tag: String, id: i64 }', 'CreateTextNode    { text: String, id: i64 }', 'SetAttribute      { id: i64, name: String, value: String }', 'RemoveAttribute   { id: i64, name: String }', 'AppendChild       { parent_id: i64, child_id: i64 }', 'InsertBefore      { parent_id: i64, new_id: i64, ref_id: i64 }', 'RemoveNode        { id: i64 }', 'MoveNode          { id: i64, new_parent_id: i64 }', 'UpdateText        { id: i64, text: String }', 'AddEventListener  { id: i64, event: String, handler_id: String, capture: bool }', 'RemoveEventListener { id: i64, event: String, handler_id: String }', 'SetStyle          { id: i64, css: String }', 'Focus             { id: i64 }', 'ScrollTo          { id: i64, x: f64, y: f64 }'], "Mutation definition"),
    ("RenderTarget", ['Desktop', 'Web', 'IOS', 'Android', 'Terminal'], "RenderTarget definition"),
    ("AppTheme", ['Light', 'Dark', 'System', 'Custom(String)'], "AppTheme definition"),
    ("Route", ['Home', 'About', 'Settings', 'Dynamic(String)', 'NotFound'], "Route definition"),
]

@dataclass
class UnifiedUiCompilerGenerator:
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
        output += "\n@dataclass\nclass Attribute:\n    pass\n\n@dataclass\nclass ContextStore:\n    pass\n\n@dataclass\nclass EventDelegator:\n    pass\n\n@dataclass\nclass EventHandler:\n    pass\n\n@dataclass\nclass Memo:\n    pass\n\n@dataclass\nclass ModifierKeys:\n    pass\n\n@dataclass\nclass NovaApp:\n    pass\n\n@dataclass\nclass Router:\n    pass\n\n@dataclass\nclass Signal:\n    pass\n\n@dataclass\nclass StyleProp:\n    pass\n\n@dataclass\nclass StyleSheet:\n    pass\n\n@dataclass\nclass TerminalRenderer:\n    pass\n\n@dataclass\nclass VirtualDom:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_UNIFIED_UI_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_UNIFIED_UI_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = UnifiedUiCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
