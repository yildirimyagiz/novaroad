#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Attribute:
    pass

@dataclass
class ContextStore:
    pass

@dataclass
class EventDelegator:
    pass

@dataclass
class EventHandler:
    pass

@dataclass
class Memo:
    pass

@dataclass
class ModifierKeys:
    pass

@dataclass
class NovaApp:
    pass

@dataclass
class Router:
    pass

@dataclass
class Signal:
    pass

@dataclass
class StyleProp:
    pass

@dataclass
class StyleSheet:
    pass

@dataclass
class TerminalRenderer:
    pass

@dataclass
class VirtualDom:
    pass

class Element(IntEnum):
    """Element definition"""
    None = 0
    Text = 1
    Node = 2
    Component = 3
    Fragment = 4
    Portal = 5

class AttrValue(IntEnum):
    """AttrValue definition"""
    Text = 6
    Number = 7
    Bool = 8
    Handler = 9
    Style = 10
    None = 11

class PropValue(IntEnum):
    """PropValue definition"""
    Str = 12
    Int = 13
    Float = 14
    Bool = 15
    List = 16
    Element = 17
    None = 18

class PlatformEvent(IntEnum):
    """PlatformEvent definition"""
    Click = 19
    DblClick = 20
    Input = 21
    Change = 22
    KeyDown = 23
    KeyUp = 24
    Scroll = 25
    Resize = 26
    Focus = 27
    Blur = 28
    Touch = 29
    Custom = 30

class TouchPhase(IntEnum):
    """TouchPhase definition"""
    Began = 31
    Moved = 32
    Ended = 33
    Cancelled = 34

class Mutation(IntEnum):
    """Mutation definition"""
    CreateElement = 35
    CreateTextNode = 36
    SetAttribute = 37
    RemoveAttribute = 38
    AppendChild = 39
    InsertBefore = 40
    RemoveNode = 41
    MoveNode = 42
    UpdateText = 43
    AddEventListener = 44
    RemoveEventListener = 45
    SetStyle = 46
    Focus = 47
    ScrollTo = 48

class RenderTarget(IntEnum):
    """RenderTarget definition"""
    Desktop = 49
    Web = 50
    IOS = 51
    Android = 52
    Terminal = 53

class AppTheme(IntEnum):
    """AppTheme definition"""
    Light = 54
    Dark = 55
    System = 56
    Custom = 57

class Route(IntEnum):
    """Route definition"""
    Home = 58
    About = 59
    Settings = 60
    Dynamic = 61
    NotFound = 62


