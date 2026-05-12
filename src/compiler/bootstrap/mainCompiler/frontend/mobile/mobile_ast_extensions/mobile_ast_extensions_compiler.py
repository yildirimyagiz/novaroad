#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("MobileExpr", ['TouchEvent(TouchEventExpr)', 'GestureEvent(GestureEventExpr)', 'DeviceSensor(DeviceSensorExpr)', 'PlatformCall(PlatformCallExpr)', 'UIComponent(UIComponentExpr)'], "MobileExpr definition"),
    ("TouchEventType", ['TouchStart', 'TouchMove', 'TouchEnd', 'TouchCancel', 'ForceChange'], "TouchEventType definition"),
    ("GestureType", ['Tap', 'DoubleTap', 'LongPress', 'Pan', 'Pinch', 'Rotate', 'Swipe'], "GestureType definition"),
    ("SensorType", ['Accelerometer', 'Gyroscope', 'Magnetometer', 'GPS', 'Barometer', 'Proximity', 'AmbientLight', 'Thermometer', 'HeartRate'], "SensorType definition"),
    ("SensorData", ['Acceleration(Point3D)', 'Rotation(Point3D)', 'MagneticField(Point3D)', 'Location(GPSLocation)', 'Pressure(f64)', 'Proximity(bool)', 'LightLevel(f64)', 'Temperature(f64)', 'HeartRate(i32)'], "SensorData definition"),
    ("UIComponentType", ['Text', 'Button', 'Image', 'TextField', 'TextArea', 'Switch', 'Slider', 'ProgressBar', 'ActivityIndicator', 'ScrollView', 'ListView', 'GridView', 'TabBar', 'NavigationBar', 'Alert', 'Modal', 'Picker', 'DatePicker', 'Custom(String)'], "UIComponentType definition"),
    ("MobileStmt", ['TouchHandler(TouchHandlerStmt)', 'GestureHandler(GestureHandlerStmt)', 'SensorHandler(SensorHandlerStmt)', 'UIUpdate(UIUpdateStmt)', 'Navigation(NavigationStmt)'], "MobileStmt definition"),
    ("NavigationAction", ['Push', 'Pop', 'Replace', 'Reset', 'Modal'], "NavigationAction definition"),
    ("MobileTarget", ['iOS(iOSTarget)', 'Android(AndroidTarget)'], "MobileTarget definition"),
    ("Permission", ['Camera', 'Location', 'Microphone', 'Photos', 'Contacts', 'Calendar', 'Reminders', 'Bluetooth', 'Network', 'Notifications', 'Health'], "Permission definition"),
]

@dataclass
class MobileAstExtensionsCompilerGenerator:
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
        output += "\n@dataclass\nclass AndroidConfig:\n    pass\n\n@dataclass\nclass AndroidTarget:\n    pass\n\n@dataclass\nclass AppManifest:\n    pass\n\n@dataclass\nclass DeviceSensorExpr:\n    pass\n\n@dataclass\nclass GPSLocation:\n    pass\n\n@dataclass\nclass GestureEventExpr:\n    pass\n\n@dataclass\nclass GestureHandlerStmt:\n    pass\n\n@dataclass\nclass MobileCodeGen:\n    pass\n\n@dataclass\nclass MobileCompiler:\n    pass\n\n@dataclass\nclass NavigationStmt:\n    pass\n\n@dataclass\nclass PlatformCallExpr:\n    pass\n\n@dataclass\nclass Point3D:\n    pass\n\n@dataclass\nclass PointExpr:\n    pass\n\n@dataclass\nclass SensorHandlerStmt:\n    pass\n\n@dataclass\nclass SigningConfig:\n    pass\n\n@dataclass\nclass TouchEventExpr:\n    pass\n\n@dataclass\nclass TouchExpr:\n    pass\n\n@dataclass\nclass TouchHandlerStmt:\n    pass\n\n@dataclass\nclass UIComponentExpr:\n    pass\n\n@dataclass\nclass UIUpdateStmt:\n    pass\n\n@dataclass\nclass iOSConfig:\n    pass\n\n@dataclass\nclass iOSTarget:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_MOBILE_AST_EXTENSIONS_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_MOBILE_AST_EXTENSIONS_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = MobileAstExtensionsCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
