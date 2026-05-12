#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


class Error(IntEnum):
    """Error definition"""
    NotFound = 0
    PermissionDenied = 1
    ConnectionRefused = 2
    ConnectionReset = 3
    ConnectionAborted = 4
    NotConnected = 5
    AddrInUse = 6
    AddrNotAvailable = 7
    BrokenPipe = 8
    AlreadyExists = 9
    WouldBlock = 10
    InvalidInput = 11
    InvalidData = 12
    TimedOut = 13
    WriteZero = 14
    Interrupted = 15
    Other = 16


