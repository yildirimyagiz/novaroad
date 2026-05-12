#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("Node", ['Element(Element)', 'Text(String)', 'Comment(String)'], "Node definition"),
    ("EventType", ['Click', 'DoubleClick', 'MouseDown', 'MouseUp', 'MouseMove', 'MouseEnter', 'MouseLeave', 'KeyDown', 'KeyUp', 'KeyPress', 'Focus', 'Blur', 'Change', 'Input', 'Submit', 'Load', 'Unload', 'Resize', 'Scroll', 'Custom(String)'], "EventType definition"),
    ("EventResult", ['Continue', 'StopPropagation', 'PreventDefault', 'Both'], "EventResult definition"),
    ("WebSocketState", ['Connecting', 'Open', 'Closing', 'Closed'], "WebSocketState definition"),
    ("WebSocketEventType", ['Open', 'Message', 'Error', 'Close'], "WebSocketEventType definition"),
    ("HttpMethod", ['GET', 'POST', 'PUT', 'DELETE', 'PATCH', 'HEAD', 'OPTIONS'], "HttpMethod definition"),
    ("ResponseType", ['Text', 'Json', 'ArrayBuffer', 'Blob'], "ResponseType definition"),
    ("DOMError", ['ElementNotFound(String)', 'InvalidSelector(String)', 'NotFound', 'HierarchyError'], "DOMError definition"),
    ("HttpError", ['NetworkError(String)', 'Timeout', 'ParseError(String)', 'StatusError(i32)'], "HttpError definition"),
    ("GeolocationError", ['PermissionDenied', 'PositionUnavailable', 'Timeout'], "GeolocationError definition"),
    ("NotificationError", ['PermissionDenied', 'NotSupported', 'InvalidOptions(String)'], "NotificationError definition"),
    ("NotificationPermission", ['Granted', 'Denied', 'Default'], "NotificationPermission definition"),
]

@dataclass
class DomBindingsCompilerGenerator:
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
        output += "\n@dataclass\nclass Element:\n    pass\n\n@dataclass\nclass EventData:\n    pass\n\n@dataclass\nclass EventListener:\n    pass\n\n@dataclass\nclass GeolocationPosition:\n    pass\n\n@dataclass\nclass HttpRequest:\n    pass\n\n@dataclass\nclass HttpResponse:\n    pass\n\n@dataclass\nclass NotificationOptions:\n    pass\n\n@dataclass\nclass WebSocket:\n    pass\n\n@dataclass\nclass WebSocketEvent:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_DOM_BINDINGS_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_DOM_BINDINGS_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = DomBindingsCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
