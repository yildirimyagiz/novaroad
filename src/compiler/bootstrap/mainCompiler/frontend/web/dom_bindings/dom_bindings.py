#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Element:
    pass

@dataclass
class EventData:
    pass

@dataclass
class EventListener:
    pass

@dataclass
class GeolocationPosition:
    pass

@dataclass
class HttpRequest:
    pass

@dataclass
class HttpResponse:
    pass

@dataclass
class NotificationOptions:
    pass

@dataclass
class WebSocket:
    pass

@dataclass
class WebSocketEvent:
    pass

class Node(IntEnum):
    """Node definition"""
    Element = 0
    Text = 1
    Comment = 2

class EventType(IntEnum):
    """EventType definition"""
    Click = 3
    DoubleClick = 4
    MouseDown = 5
    MouseUp = 6
    MouseMove = 7
    MouseEnter = 8
    MouseLeave = 9
    KeyDown = 10
    KeyUp = 11
    KeyPress = 12
    Focus = 13
    Blur = 14
    Change = 15
    Input = 16
    Submit = 17
    Load = 18
    Unload = 19
    Resize = 20
    Scroll = 21
    Custom = 22

class EventResult(IntEnum):
    """EventResult definition"""
    Continue = 23
    StopPropagation = 24
    PreventDefault = 25
    Both = 26

class WebSocketState(IntEnum):
    """WebSocketState definition"""
    Connecting = 27
    Open = 28
    Closing = 29
    Closed = 30

class WebSocketEventType(IntEnum):
    """WebSocketEventType definition"""
    Open = 31
    Message = 32
    Error = 33
    Close = 34

class HttpMethod(IntEnum):
    """HttpMethod definition"""
    GET = 35
    POST = 36
    PUT = 37
    DELETE = 38
    PATCH = 39
    HEAD = 40
    OPTIONS = 41

class ResponseType(IntEnum):
    """ResponseType definition"""
    Text = 42
    Json = 43
    ArrayBuffer = 44
    Blob = 45

class DOMError(IntEnum):
    """DOMError definition"""
    ElementNotFound = 46
    InvalidSelector = 47
    NotFound = 48
    HierarchyError = 49

class HttpError(IntEnum):
    """HttpError definition"""
    NetworkError = 50
    Timeout = 51
    ParseError = 52
    StatusError = 53

class GeolocationError(IntEnum):
    """GeolocationError definition"""
    PermissionDenied = 54
    PositionUnavailable = 55
    Timeout = 56

class NotificationError(IntEnum):
    """NotificationError definition"""
    PermissionDenied = 57
    NotSupported = 58
    InvalidOptions = 59

class NotificationPermission(IntEnum):
    """NotificationPermission definition"""
    Granted = 60
    Denied = 61
    Default = 62


