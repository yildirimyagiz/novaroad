#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("CursorType", ['Arrow', 'IBeam', 'Crosshair', 'Hand', 'HResize', 'VResize', 'NWResize', 'NEResize', 'SWResize', 'SEResize', 'Wait', 'None'], "CursorType definition"),
    ("EventType", ['WindowClose', 'WindowResize', 'WindowMove', 'WindowFocus', 'WindowBlur', 'KeyPress', 'KeyRelease', 'MouseMove', 'MouseButtonPress', 'MouseButtonRelease', 'MouseScroll', 'TextInput', 'FileDrop', 'Custom(String)'], "EventType definition"),
    ("EventData", ['WindowEvent(WindowEventData)', 'KeyboardEvent(KeyboardEventData)', 'MouseEvent(MouseEventData)', 'TextEvent(String)', 'FileDropEvent(Vec<String>)', 'CustomEvent(HashMap<String, String>)'], "EventData definition"),
    ("WindowEventType", ['Closed', 'Resized', 'Moved', 'Focused', 'Blurred', 'Minimized', 'Maximized', 'Restored'], "WindowEventType definition"),
    ("Key", ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'Num0', 'Num1', 'Num2', 'Num3', 'Num4', 'Num5', 'Num6', 'Num7', 'Num8', 'Num9', 'Escape', 'F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'F8', 'F9', 'F10', 'F11', 'F12', 'PrintScreen', 'ScrollLock', 'Pause', 'Insert', 'Delete', 'Home', 'End', 'PageUp', 'PageDown', 'Left', 'Right', 'Up', 'Down', 'Backspace', 'Tab', 'Return', 'Space', 'LShift', 'RShift', 'LCtrl', 'RCtrl', 'LAlt', 'RAlt', 'LSuper', 'RSuper', 'Menu', 'CapsLock', 'NumLock'], "Key definition"),
    ("MouseButton", ['Left', 'Right', 'Middle', 'X1', 'X2'], "MouseButton definition"),
    ("EventResult", ['Continue', 'Handled', 'StopPropagation'], "EventResult definition"),
    ("WindowError", ['CreationFailed', 'InvalidConfig', 'PlatformError'], "WindowError definition"),
    ("AppError", ['InitializationFailed', 'WindowCreationFailed', 'EventLoopError'], "AppError definition"),
]

@dataclass
class WindowSystemCompilerGenerator:
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
        output += "\n@dataclass\nclass AppConfig:\n    pass\n\n@dataclass\nclass CEvent:\n    pass\n\n@dataclass\nclass CMonitorInfo:\n    pass\n\n@dataclass\nclass Color:\n    pass\n\n@dataclass\nclass DesktopApp:\n    pass\n\n@dataclass\nclass EventLoop:\n    pass\n\n@dataclass\nclass Icon:\n    pass\n\n@dataclass\nclass KeyModifiers:\n    pass\n\n@dataclass\nclass KeyboardEventData:\n    pass\n\n@dataclass\nclass MonitorInfo:\n    pass\n\n@dataclass\nclass MouseEventData:\n    pass\n\n@dataclass\nclass Point:\n    pass\n\n@dataclass\nclass Size:\n    pass\n\n@dataclass\nclass Window:\n    pass\n\n@dataclass\nclass WindowConfig:\n    pass\n\n@dataclass\nclass WindowEventData:\n    pass\n\n@dataclass\nclass WindowId:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_WINDOW_SYSTEM_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_WINDOW_SYSTEM_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = WindowSystemCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
