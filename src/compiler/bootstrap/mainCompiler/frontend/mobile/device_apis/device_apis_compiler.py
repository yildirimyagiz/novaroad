#!/usr/bin/env python3
from __future__ import annotations
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple

# ═══════════════════════════════════════════════════════════════════════════
# §1  COMBINED VARIANT DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════

CASES: List[Tuple[str, List[str], str]] = [
    ("SensorType", ['Accelerometer', 'Gyroscope', 'Magnetometer', 'GPS', 'Barometer', 'Proximity', 'AmbientLight', 'Thermometer', 'HeartRate'], "SensorType definition"),
    ("SensorData", ['Acceleration(Point3D)', 'Rotation(Point3D)', 'MagneticField(Point3D)', 'Location(GPSData)', 'Pressure(f64)', 'Proximity(bool)', 'LightLevel(f64)', 'Temperature(f64)', 'HeartRate(i32)'], "SensorData definition"),
    ("SensorError", ['NotAvailable', 'PermissionDenied', 'HardwareFailure', 'Timeout'], "SensorError definition"),
    ("CameraPosition", ['Front', 'Back'], "CameraPosition definition"),
    ("FlashMode", ['Off', 'On', 'Auto'], "FlashMode definition"),
    ("FocusMode", ['Auto', 'Manual', 'Continuous'], "FocusMode definition"),
    ("CameraError", ['NotAvailable', 'PermissionDenied', 'HardwareFailure', 'InvalidConfiguration'], "CameraError definition"),
    ("NotificationAction", ['Opened', 'Dismissed', 'Custom(String)'], "NotificationAction definition"),
    ("NotificationError", ['PermissionDenied', 'SchedulingFailed', 'NotFound', 'SystemError'], "NotificationError definition"),
    ("StorageError", ['FileNotFound', 'DirectoryNotFound', 'PermissionDenied', 'WriteFailed', 'ReadFailed', 'CreateFailed', 'DeleteFailed'], "StorageError definition"),
    ("ConnectionType", ['None', 'WiFi', 'Cellular', 'Ethernet', 'Unknown'], "ConnectionType definition"),
    ("HTTPMethod", ['GET', 'POST', 'PUT', 'DELETE', 'PATCH'], "HTTPMethod definition"),
    ("NetworkError", ['NoConnection', 'Timeout', 'InvalidURL', 'ServerError', 'Unknown'], "NetworkError definition"),
]

@dataclass
class DeviceApisCompilerGenerator:
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
        output += "\n@dataclass\nclass CameraConfig:\n    pass\n\n@dataclass\nclass CameraManager:\n    pass\n\n@dataclass\nclass DeviceAPI:\n    pass\n\n@dataclass\nclass DeviceCapabilities:\n    pass\n\n@dataclass\nclass FileInfo:\n    pass\n\n@dataclass\nclass FileInfoRaw:\n    pass\n\n@dataclass\nclass GPSData:\n    pass\n\n@dataclass\nclass HTTPRequest:\n    pass\n\n@dataclass\nclass HTTPResponse:\n    pass\n\n@dataclass\nclass ImageMetadata:\n    pass\n\n@dataclass\nclass NetworkManager:\n    pass\n\n@dataclass\nclass NotificationManager:\n    pass\n\n@dataclass\nclass NotificationRequest:\n    pass\n\n@dataclass\nclass NotificationResponse:\n    pass\n\n@dataclass\nclass Point3D:\n    pass\n\n@dataclass\nclass Resolution:\n    pass\n\n@dataclass\nclass SensorManager:\n    pass\n\n@dataclass\nclass StorageManager:\n    pass\n\n"
        for group_name, variants, doc in self.cases:
            output += f'class {group_name}(IntEnum):\n    """{doc}"""\n'
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip()
                output += f'    {v_clean} = {tag}\n'
            output += "\n"
        return output

    def emit_c_header(self) -> str:
        guard = f"NOVA_DEVICE_APIS_TAGS_H"
        output = f"#ifndef {guard}\n#define {guard}\n\n"
        for group_name, variants, doc in self.cases:
            output += f"/* {doc} */\n"
            for var in variants:
                tag = self.tag_map[f"{group_name}::{var}"]
                v_clean = var.split("(")[0].split("{")[0].strip().upper()
                output += f"#define NOVA_DEVICE_APIS_{group_name.upper()}_{v_clean} {tag}\n"
            output += "\n"
        output += "#endif\n"
        return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--emit-python", action="store_true")
    p.add_argument("--emit-c", action="store_true")
    args = p.parse_args()
    c = DeviceApisCompilerGenerator().compile()
    if args.emit_python: print(c.emit_python_enums())
    elif args.emit_c: print(c.emit_c_header())

if __name__ == "__main__": main()
