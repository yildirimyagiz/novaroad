#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AndroidConfig:
    pass

@dataclass
class AndroidTarget:
    pass

@dataclass
class AppManifest:
    pass

@dataclass
class DeviceSensorExpr:
    pass

@dataclass
class GPSLocation:
    pass

@dataclass
class GestureEventExpr:
    pass

@dataclass
class GestureHandlerStmt:
    pass

@dataclass
class MobileCodeGen:
    pass

@dataclass
class MobileCompiler:
    pass

@dataclass
class NavigationStmt:
    pass

@dataclass
class PlatformCallExpr:
    pass

@dataclass
class Point3D:
    pass

@dataclass
class PointExpr:
    pass

@dataclass
class SensorHandlerStmt:
    pass

@dataclass
class SigningConfig:
    pass

@dataclass
class TouchEventExpr:
    pass

@dataclass
class TouchExpr:
    pass

@dataclass
class TouchHandlerStmt:
    pass

@dataclass
class UIComponentExpr:
    pass

@dataclass
class UIUpdateStmt:
    pass

@dataclass
class iOSConfig:
    pass

@dataclass
class iOSTarget:
    pass

class MobileExpr(IntEnum):
    """MobileExpr definition"""
    TouchEvent = 0
    GestureEvent = 1
    DeviceSensor = 2
    PlatformCall = 3
    UIComponent = 4

class TouchEventType(IntEnum):
    """TouchEventType definition"""
    TouchStart = 5
    TouchMove = 6
    TouchEnd = 7
    TouchCancel = 8
    ForceChange = 9

class GestureType(IntEnum):
    """GestureType definition"""
    Tap = 10
    DoubleTap = 11
    LongPress = 12
    Pan = 13
    Pinch = 14
    Rotate = 15
    Swipe = 16

class SensorType(IntEnum):
    """SensorType definition"""
    Accelerometer = 17
    Gyroscope = 18
    Magnetometer = 19
    GPS = 20
    Barometer = 21
    Proximity = 22
    AmbientLight = 23
    Thermometer = 24
    HeartRate = 25

class SensorData(IntEnum):
    """SensorData definition"""
    Acceleration = 26
    Rotation = 27
    MagneticField = 28
    Location = 29
    Pressure = 30
    Proximity = 31
    LightLevel = 32
    Temperature = 33
    HeartRate = 34

class UIComponentType(IntEnum):
    """UIComponentType definition"""
    Text = 35
    Button = 36
    Image = 37
    TextField = 38
    TextArea = 39
    Switch = 40
    Slider = 41
    ProgressBar = 42
    ActivityIndicator = 43
    ScrollView = 44
    ListView = 45
    GridView = 46
    TabBar = 47
    NavigationBar = 48
    Alert = 49
    Modal = 50
    Picker = 51
    DatePicker = 52
    Custom = 53

class MobileStmt(IntEnum):
    """MobileStmt definition"""
    TouchHandler = 54
    GestureHandler = 55
    SensorHandler = 56
    UIUpdate = 57
    Navigation = 58

class NavigationAction(IntEnum):
    """NavigationAction definition"""
    Push = 59
    Pop = 60
    Replace = 61
    Reset = 62
    Modal = 63

class MobileTarget(IntEnum):
    """MobileTarget definition"""
    iOS = 64
    Android = 65

class Permission(IntEnum):
    """Permission definition"""
    Camera = 66
    Location = 67
    Microphone = 68
    Photos = 69
    Contacts = 70
    Calendar = 71
    Reminders = 72
    Bluetooth = 73
    Network = 74
    Notifications = 75
    Health = 76


