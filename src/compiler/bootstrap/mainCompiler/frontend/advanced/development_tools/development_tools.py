#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AssetReloadHandler:
    pass

@dataclass
class Breakpoint:
    pass

@dataclass
class BuildConfig:
    pass

@dataclass
class CodeReloadHandler:
    pass

@dataclass
class CoverageAnalyzer:
    pass

@dataclass
class CoverageData:
    pass

@dataclass
class CoverageDataRaw:
    pass

@dataclass
class CoverageReport:
    pass

@dataclass
class DebugEvent:
    pass

@dataclass
class DebugException:
    pass

@dataclass
class DebugValue:
    pass

@dataclass
class Debugger:
    pass

@dataclass
class DevelopmentProfiler:
    pass

@dataclass
class DevelopmentStatus:
    pass

@dataclass
class EvalResult:
    pass

@dataclass
class FileCoverageReport:
    pass

@dataclass
class FunctionCall:
    pass

@dataclass
class FunctionInfo:
    pass

@dataclass
class HotReloadEngine:
    pass

@dataclass
class LNIde:
    pass

@dataclass
class LanguageServer:
    pass

@dataclass
class PerformanceSample:
    pass

@dataclass
class PerformanceSampleRaw:
    pass

@dataclass
class ProfilingReport:
    pass

@dataclass
class ProjectConfig:
    pass

@dataclass
class ReloadChange:
    pass

@dataclass
class ReloadRequest:
    pass

@dataclass
class ServerCapabilities:
    pass

@dataclass
class SourceFile:
    pass

@dataclass
class StackFrame:
    pass

@dataclass
class StackFrameRaw:
    pass

class ChangeType(IntEnum):
    """ChangeType definition"""
    Modified = 0
    Added = 1
    Deleted = 2
    Renamed = 3

class ReloadResult(IntEnum):
    """ReloadResult definition"""
    Success = 4
    Failure = 5
    NotApplicable = 6

class ExecutionState(IntEnum):
    """ExecutionState definition"""
    Running = 7
    Paused = 8
    Terminated = 9

class BreakReason(IntEnum):
    """BreakReason definition"""
    BreakpointHit = 10
    StepOver = 11
    StepInto = 12
    StepOut = 13
    Exception = 14
    UserRequest = 15

class DebugError(IntEnum):
    """DebugError definition"""
    NotAttached = 16
    AttachFailed = 17
    DetachFailed = 18
    BreakpointError = 19
    ContinueFailed = 20
    PauseFailed = 21
    StepFailed = 22
    EvaluationFailed = 23
    InvalidExpression = 24

class ProfileError(IntEnum):
    """ProfileError definition"""
    ExportFailed = 25
    RecordingNotActive = 26
    InsufficientData = 27

class DevError(IntEnum):
    """DevError definition"""
    BuildFailed = 28
    RunFailed = 29
    HotReloadFailed = 30
    DebugError = 31


