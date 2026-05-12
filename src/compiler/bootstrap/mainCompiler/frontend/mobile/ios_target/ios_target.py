#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AssetCompiler:
    pass

@dataclass
class IOSCodeGenerator:
    pass

@dataclass
class IOSTarget:
    pass

@dataclass
class SwiftCodeGenerator:
    pass

@dataclass
class XcodeProject:
    pass

@dataclass
class XcodeProjectGenerator:
    pass

class IOSDeviceFamily(IntEnum):
    """IOSDeviceFamily definition"""
    IPhone = 0
    IPad = 1
    Universal = 2
    MacCatalyst = 3

class CompilationError(IntEnum):
    """CompilationError definition"""
    SwiftGenerationError = 4
    XcodeProjectError = 5
    AssetCompilationError = 6

class ProjectError(IntEnum):
    """ProjectError definition"""
    InvalidBundleId = 7
    MissingProvisioningProfile = 8
    CodeSigningError = 9


