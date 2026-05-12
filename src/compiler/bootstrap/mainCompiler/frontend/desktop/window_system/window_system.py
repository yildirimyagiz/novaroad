#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AppConfig:
    pass

@dataclass
class CEvent:
    pass

@dataclass
class CMonitorInfo:
    pass

@dataclass
class Color:
    pass

@dataclass
class DesktopApp:
    pass

@dataclass
class EventLoop:
    pass

@dataclass
class Icon:
    pass

@dataclass
class KeyModifiers:
    pass

@dataclass
class KeyboardEventData:
    pass

@dataclass
class MonitorInfo:
    pass

@dataclass
class MouseEventData:
    pass

@dataclass
class Point:
    pass

@dataclass
class Size:
    pass

@dataclass
class Window:
    pass

@dataclass
class WindowConfig:
    pass

@dataclass
class WindowEventData:
    pass

@dataclass
class WindowId:
    pass

class CursorType(IntEnum):
    """CursorType definition"""
    Arrow = 0
    IBeam = 1
    Crosshair = 2
    Hand = 3
    HResize = 4
    VResize = 5
    NWResize = 6
    NEResize = 7
    SWResize = 8
    SEResize = 9
    Wait = 10
    None = 11

class EventType(IntEnum):
    """EventType definition"""
    WindowClose = 12
    WindowResize = 13
    WindowMove = 14
    WindowFocus = 15
    WindowBlur = 16
    KeyPress = 17
    KeyRelease = 18
    MouseMove = 19
    MouseButtonPress = 20
    MouseButtonRelease = 21
    MouseScroll = 22
    TextInput = 23
    FileDrop = 24
    Custom = 25

class EventData(IntEnum):
    """EventData definition"""
    WindowEvent = 26
    KeyboardEvent = 27
    MouseEvent = 28
    TextEvent = 29
    FileDropEvent = 30
    CustomEvent = 31

class WindowEventType(IntEnum):
    """WindowEventType definition"""
    Closed = 32
    Resized = 33
    Moved = 34
    Focused = 35
    Blurred = 36
    Minimized = 37
    Maximized = 38
    Restored = 39

class Key(IntEnum):
    """Key definition"""
    A = 40
    B = 41
    C = 42
    D = 43
    E = 44
    F = 45
    G = 46
    H = 47
    I = 48
    J = 49
    K = 50
    L = 51
    M = 52
    N = 53
    O = 54
    P = 55
    Q = 56
    R = 57
    S = 58
    T = 59
    U = 60
    V = 61
    W = 62
    X = 63
    Y = 64
    Z = 65
    Num0 = 66
    Num1 = 67
    Num2 = 68
    Num3 = 69
    Num4 = 70
    Num5 = 71
    Num6 = 72
    Num7 = 73
    Num8 = 74
    Num9 = 75
    Escape = 76
    F1 = 77
    F2 = 78
    F3 = 79
    F4 = 80
    F5 = 81
    F6 = 82
    F7 = 83
    F8 = 84
    F9 = 85
    F10 = 86
    F11 = 87
    F12 = 88
    PrintScreen = 89
    ScrollLock = 90
    Pause = 91
    Insert = 92
    Delete = 93
    Home = 94
    End = 95
    PageUp = 96
    PageDown = 97
    Left = 98
    Right = 99
    Up = 100
    Down = 101
    Backspace = 102
    Tab = 103
    Return = 104
    Space = 105
    LShift = 106
    RShift = 107
    LCtrl = 108
    RCtrl = 109
    LAlt = 110
    RAlt = 111
    LSuper = 112
    RSuper = 113
    Menu = 114
    CapsLock = 115
    NumLock = 116

class MouseButton(IntEnum):
    """MouseButton definition"""
    Left = 117
    Right = 118
    Middle = 119
    X1 = 120
    X2 = 121

class EventResult(IntEnum):
    """EventResult definition"""
    Continue = 122
    Handled = 123
    StopPropagation = 124

class WindowError(IntEnum):
    """WindowError definition"""
    CreationFailed = 125
    InvalidConfig = 126
    PlatformError = 127

class AppError(IntEnum):
    """AppError definition"""
    InitializationFailed = 128
    WindowCreationFailed = 129
    EventLoopError = 130


