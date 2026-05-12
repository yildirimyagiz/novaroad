#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Client:
    pass

@dataclass
class Request:
    pass

@dataclass
class Response:
    pass

class Method(IntEnum):
    """Method definition"""
    GET = 0
    POST = 1
    PUT = 2
    DELETE = 3
    PATCH = 4
    HEAD = 5
    OPTIONS = 6

class StatusCode(IntEnum):
    """StatusCode definition"""
    OK = 200 = 7
    Created = 201 = 8
    NoContent = 204 = 9
    MovedPermanently = 301 = 10
    Found = 302 = 11
    BadRequest = 400 = 12
    Unauthorized = 401 = 13
    Forbidden = 403 = 14
    NotFound = 404 = 15
    InternalServerError = 500 = 16


