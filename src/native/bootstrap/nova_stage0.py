#!/usr/bin/env python3
"""
Nova - Stage 0 Bootstrap Compiler
=========================================
.zn kaynak kodu → C kodu → Native binary

Bu compiler:
  1. Nova (.zn) lexer
  2. Nova parser (AST)
  3. C code generator
  4. GCC ile native derleme

Stage 1 ile self-hosting mümkün olacak.
"""

import sys
import os
import re
import subprocess
import tempfile
from dataclasses import dataclass, field
from typing import List, Optional, Dict, Any, Tuple
from enum import Enum, auto

# ===========================================================================
# TOKEN TYPES
# ===========================================================================


class TT(Enum):
    # Literals
    INT_LIT = auto()
    FLOAT_LIT = auto()
    STR_LIT = auto()
    BOOL_LIT = auto()
    # Identifiers and keywords
    IDENT = auto()
    # Keywords
    KW_FN = auto()
    KW_LET = auto()
    KW_MUT = auto()
    KW_IF = auto()
    KW_ELSE = auto()
    KW_WHILE = auto()
    KW_FOR = auto()
    KW_IN = auto()
    KW_RETURN = auto()
    KW_STRUCT = auto()
    KW_IMPL = auto()
    KW_USE = auto()
    KW_PUB = auto()
    KW_SELF = auto()
    KW_TRUE = auto()
    KW_FALSE = auto()
    KW_MATCH = auto()
    KW_ENUM = auto()
    KW_EXTERN = auto()
    KW_BREAK = auto()
    KW_CONTINUE = auto()
    KW_YIELD = auto()
    KW_CHECK = auto()
    KW_VAR = auto()
    KW_EXPOSE = auto()
    KW_DATA = auto()
    KW_CASES = auto()
    KW_OPEN = auto()
    KW_EACH = auto()
    KW_GIVEN = auto()
    KW_ON = auto()
    KW_RULES = auto()
    KW_ALIAS = auto()
    KW_FLOW = auto()
    KW_SPAWN = auto()
    KW_AS = auto()
    KW_KIND = auto()
    KW_CRATE = auto()
    KW_UNSAFE = auto()
    KW_CONST = auto()
    KW_SPACE = auto()
    KW_TEST = auto()
    KW_SUPER = auto()
    KW_ABORT = auto()
    KW_DYN = auto()
    KW_DROP = auto()
    KW_WHERE = auto()
    KW_TYPE = auto()
    KW_TRAIT = auto()
    KW_MOD = auto()
    KW_ASYNC = auto()
    KW_AWAIT = auto()
    KW_TRY = auto()
    KW_CATCH = auto()
    KW_THROW = auto()
    KW_MOVE = auto()
    KW_REF = auto()
    KW_MACRO = auto()

    # Operators
    PLUS = auto()
    MINUS = auto()
    STAR = auto()
    SLASH = auto()
    PERCENT = auto()
    EQ = auto()
    EQ_EQ = auto()
    BANG_EQ = auto()
    LT = auto()
    GT = auto()
    LT_EQ = auto()
    GT_EQ = auto()
    AND_AND = auto()
    OR_OR = auto()
    BANG = auto()
    AMPERSAND = auto()
    PIPE = auto()
    PLUS_EQ = auto()
    MINUS_EQ = auto()
    STAR_EQ = auto()
    SLASH_EQ = auto()
    ARROW = auto()
    FAT_ARROW = auto()
    DOT = auto()
    DOT_DOT = auto()
    COLON = auto()
    COLON_COLON = auto()
    SEMICOLON = auto()
    COMMA = auto()
    LPAREN = auto()
    RPAREN = auto()
    LBRACE = auto()
    RBRACE = auto()
    LBRACKET = auto()
    RBRACKET = auto()
    QUESTION = auto()
    # Special
    EOF = auto()


KEYWORDS = {
    "fn": TT.KW_FN,
    "let": TT.KW_LET,
    "mut": TT.KW_MUT,
    "if": TT.KW_IF,
    "else": TT.KW_ELSE,
    "while": TT.KW_WHILE,
    "for": TT.KW_FOR,
    "in": TT.KW_IN,
    "return": TT.KW_RETURN,
    "struct": TT.KW_STRUCT,
    "impl": TT.KW_IMPL,
    "use": TT.KW_USE,
    "pub": TT.KW_PUB,
    "self": TT.KW_SELF,
    "true": TT.KW_TRUE,
    "false": TT.KW_FALSE,
    "match": TT.KW_MATCH,
    "enum": TT.KW_ENUM,
    "extern": TT.KW_EXTERN,
    "break": TT.KW_BREAK,
    "continue": TT.KW_CONTINUE,
    "yield": TT.KW_YIELD,
    "check": TT.KW_CHECK,
    "var": TT.KW_VAR,
    "expose": TT.KW_EXPOSE,
    "data": TT.KW_DATA,
    "cases": TT.KW_CASES,
    "open": TT.KW_OPEN,
    "each": TT.KW_EACH,
    "given": TT.KW_GIVEN,
    "on": TT.KW_ON,
    "rules": TT.KW_RULES,
    "alias": TT.KW_ALIAS,
    "flow": TT.KW_FLOW,
    "spawn": TT.KW_SPAWN,
    "as": TT.KW_AS,
    "kind": TT.KW_KIND,
    "crate": TT.KW_CRATE,
    "unsafe": TT.KW_UNSAFE,
    "const": TT.KW_CONST,
    "space": TT.KW_SPACE,
    "test": TT.KW_TEST,
    "super": TT.KW_SUPER,
    "abort": TT.KW_ABORT,
    "dyn": TT.KW_DYN,
    "drop": TT.KW_DROP,
    "where": TT.KW_WHERE,
    "type": TT.KW_TYPE,
    "trait": TT.KW_TRAIT,
    "mod": TT.KW_MOD,
    "async": TT.KW_ASYNC,
    "await": TT.KW_AWAIT,
    "try": TT.KW_TRY,
    "catch": TT.KW_CATCH,
    "throw": TT.KW_THROW,
    "move": TT.KW_MOVE,
    "ref": TT.KW_REF,
    "macro": TT.KW_MACRO,
}


@dataclass
class Token:
    type: TT
    value: Any
    line: int
    col: int

    def __repr__(self):
        return f"Token({self.type.name}, {self.value!r}, {self.line}:{self.col})"


# ===========================================================================
# LEXER
# ===========================================================================


class LexError(Exception):
    pass


class Lexer:
    def __init__(self, source: str):
        self.source = source
        self.pos = 0
        self.line = 1
        self.col = 1
        self.tokens: List[Token] = []

    def error(self, msg):
        raise LexError(f"Lex error at {self.line}:{self.col}: {msg}")

    def peek(self, offset=0) -> str:
        i = self.pos + offset
        return self.source[i] if i < len(self.source) else "\0"

    def advance(self) -> str:
        ch = self.source[self.pos]
        self.pos += 1
        if ch == "\n":
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return ch

    def emit(self, type: TT, value=None):
        self.tokens.append(Token(type, value, self.line, self.col))

    def skip_whitespace_and_comments(self):
        while self.pos < len(self.source):
            ch = self.peek()
            if ch in " \t\r\n":
                self.advance()
            elif ch == "/" and self.peek(1) == "/":
                while self.pos < len(self.source) and self.peek() != "\n":
                    self.advance()
            elif ch == "/" and self.peek(1) == "*":
                self.advance()
                self.advance()
                while self.pos < len(self.source):
                    if self.peek() == "*" and self.peek(1) == "/":
                        self.advance()
                        self.advance()
                        break
                    self.advance()
            else:
                break

    def read_string(self) -> str:
        self.advance()  # consume "
        result = []
        while self.pos < len(self.source):
            ch = self.advance()
            if ch == '"':
                return "".join(result)
            elif ch == "\\":
                esc = self.advance()
                result.append(
                    {"n": "\n", "t": "\t", "r": "\r", "\\": "\\", '"': '"'}.get(
                        esc, esc
                    )
                )
            else:
                result.append(ch)
        self.error("Unterminated string literal")

    def read_number(self) -> Token:
        line, col = self.line, self.col
        num = []
        is_float = False
        while self.pos < len(self.source) and (
            (self.peek() in "0123456789") or self.peek() == "."
        ):
            if self.peek() == ".":
                if self.peek(1) == ".":  # range ..
                    break
                is_float = True
            num.append(self.advance())
        val = "".join(num)
        if is_float:
            return Token(TT.FLOAT_LIT, float(val), line, col)
        return Token(TT.INT_LIT, int(val), line, col)

    def tokenize(self) -> List[Token]:
        while self.pos < len(self.source):
            self.skip_whitespace_and_comments()
            if self.pos >= len(self.source):
                break

            line, col = self.line, self.col
            ch = self.peek()

            # String literal
            if ch == '"':
                s = self.read_string()
                self.tokens.append(Token(TT.STR_LIT, s, line, col))
                continue

            # Number (ASCII digits only; avoid Unicode ² etc.)
            if ch in "0123456789":
                self.tokens.append(self.read_number())
                continue

            # Identifier or keyword
            if ch.isalpha() or ch == "_":
                ident = []
                while self.pos < len(self.source) and (
                    self.peek().isalnum() or self.peek() == "_"
                ):
                    ident.append(self.advance())
                word = "".join(ident)
                if word == "true":
                    self.tokens.append(Token(TT.BOOL_LIT, True, line, col))
                elif word == "false":
                    self.tokens.append(Token(TT.BOOL_LIT, False, line, col))
                elif word in KEYWORDS:
                    self.tokens.append(Token(KEYWORDS[word], word, line, col))
                else:
                    self.tokens.append(Token(TT.IDENT, word, line, col))
                continue

            # Multi-char operators
            self.advance()
            two = ch + self.peek()
            THREE_MAP = {}
            TWO_MAP = {
                "==": TT.EQ_EQ,
                "!=": TT.BANG_EQ,
                "<=": TT.LT_EQ,
                ">=": TT.GT_EQ,
                "&&": TT.AND_AND,
                "||": TT.OR_OR,
                "->": TT.ARROW,
                "=>": TT.FAT_ARROW,
                "::": TT.COLON_COLON,
                "..": TT.DOT_DOT,
                "+=": TT.PLUS_EQ,
                "-=": TT.MINUS_EQ,
                "*=": TT.STAR_EQ,
                "/=": TT.SLASH_EQ,
            }
            ONE_MAP = {
                "+": TT.PLUS,
                "-": TT.MINUS,
                "*": TT.STAR,
                "/": TT.SLASH,
                "%": TT.PERCENT,
                "=": TT.EQ,
                "<": TT.LT,
                ">": TT.GT,
                "!": TT.BANG,
                "&": TT.AMPERSAND,
                "|": TT.PIPE,
                ".": TT.DOT,
                ":": TT.COLON,
                ";": TT.SEMICOLON,
                ",": TT.COMMA,
                "(": TT.LPAREN,
                ")": TT.RPAREN,
                "{": TT.LBRACE,
                "}": TT.RBRACE,
                "[": TT.LBRACKET,
                "]": TT.RBRACKET,
                "?": TT.QUESTION,
            }
            if two in TWO_MAP:
                self.advance()
                self.tokens.append(Token(TWO_MAP[two], two, line, col))
            elif ch in ONE_MAP:
                self.tokens.append(Token(ONE_MAP[ch], ch, line, col))
            # else: skip unknown char

        self.tokens.append(Token(TT.EOF, None, self.line, self.col))
        return self.tokens


# ===========================================================================
# AST NODES
# ===========================================================================


@dataclass
class ASTNode:
    pass


@dataclass
class Program(ASTNode):
    items: List[ASTNode]


@dataclass
class UseDecl(ASTNode):
    path: str


@dataclass
class FnDecl(ASTNode):
    name: str
    params: List[Tuple[str, str]]  # (name, type)
    ret_type: Optional[str]
    body: List[ASTNode]
    is_pub: bool = False
    self_param: bool = False


@dataclass
class StructDecl(ASTNode):
    name: str
    fields: List[Tuple[str, str]]


@dataclass
class ImplBlock(ASTNode):
    type_name: str
    methods: List[FnDecl]


@dataclass
class EnumDecl(ASTNode):
    name: str
    variants: List[str]


# Statements
@dataclass
class LetStmt(ASTNode):
    name: Any  # str or List[str] pattern
    type_ann: Optional[str]
    value: Optional[ASTNode]
    is_mut: bool = False


@dataclass
class ReturnStmt(ASTNode):
    value: Optional[ASTNode]


@dataclass
class ExprStmt(ASTNode):
    expr: ASTNode


@dataclass
class IfStmt(ASTNode):
    cond: ASTNode
    then_block: List[ASTNode]
    else_block: Optional[List[ASTNode]]


@dataclass
class WhileStmt(ASTNode):
    cond: ASTNode
    body: List[ASTNode]


@dataclass
class ForStmt(ASTNode):
    var: str
    iter: ASTNode
    body: List[ASTNode]


@dataclass
class BreakStmt(ASTNode):
    pass


@dataclass
class ContinueStmt(ASTNode):
    pass


# Expressions
@dataclass
class IntLit(ASTNode):
    value: int


@dataclass
class FloatLit(ASTNode):
    value: float


@dataclass
class StrLit(ASTNode):
    value: str


@dataclass
class BoolLit(ASTNode):
    value: bool


@dataclass
class Identifier(ASTNode):
    name: str


@dataclass
class BinOp(ASTNode):
    op: str
    left: ASTNode
    right: ASTNode


@dataclass
class UnaryOp(ASTNode):
    op: str
    expr: ASTNode


@dataclass
class Assign(ASTNode):
    target: ASTNode
    op: str  # =, +=, -=, etc.
    value: ASTNode


@dataclass
class Call(ASTNode):
    func: ASTNode
    args: List[ASTNode]


@dataclass
class FieldAccess(ASTNode):
    obj: ASTNode
    field: str


@dataclass
class Index(ASTNode):
    obj: ASTNode
    idx: ASTNode


@dataclass
class MethodCall(ASTNode):
    obj: ASTNode
    method: str
    args: List[ASTNode]


@dataclass
class Block(ASTNode):
    stmts: List[ASTNode]


@dataclass
class StructLit(ASTNode):
    name: str
    fields: List[Tuple[str, ASTNode]]


@dataclass
class RangeLit(ASTNode):
    start: ASTNode
    end: ASTNode


@dataclass
class FormatStr(ASTNode):
    """format!("...", args)"""

    template: str
    args: List[ASTNode]


# ===========================================================================
# PARSER
# ===========================================================================


class ParseError(Exception):
    pass


class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.pos = 0

    def error(self, msg):
        t = self.current()
        raise ParseError(
            f"Parse error at {t.line}:{t.col} ({t.type.name} '{t.value}'): {msg}"
        )

    def current(self) -> Token:
        return self.tokens[self.pos]

    def peek(self, offset=1) -> Token:
        i = self.pos + offset
        return self.tokens[i] if i < len(self.tokens) else self.tokens[-1]

    def check(self, *types) -> bool:
        return self.current().type in types

    def match(self, *types) -> Optional[Token]:
        if self.current().type in types:
            t = self.current()
            self.pos += 1
            return t
        return None

    def expect(self, type: TT, msg: str = None) -> Token:
        t = self.match(type)
        if t is None:
            self.error(msg or f"Expected {type.name}, got {self.current().type.name}")
        return t

    def expect_ident(self) -> Token:
        t = self.current()
        if t.type == TT.IDENT:
            self.pos += 1
            return t
        if t.type.name.startswith("KW_"):
            # Allow keywords as identifiers in bootstrap
            self.pos += 1
            return t
        self.error(f"Expected identifier, got {t.type.name}")

    def skip_where_clause(self):
        if self.match(TT.KW_WHERE):
            while (
                not self.check(TT.LBRACE)
                and not self.check(TT.SEMICOLON)
                and not self.check(TT.EOF)
            ):
                self.pos += 1

    def parse(self) -> Program:
        items = []
        while not self.check(TT.EOF):
            item = self.parse_top_level()
            if item:
                items.append(item)
        return Program(items)

    def parse_top_level(self) -> Optional[ASTNode]:
        # Skip 'use' and 'mod' and 'space' declarations
        if self.match(TT.KW_USE, TT.KW_MOD, TT.KW_SPACE):
            self.advance_to_semicolon()
            return None

        is_pub = bool(self.match(TT.KW_PUB, TT.KW_EXPOSE, TT.KW_OPEN))

        if self.check(TT.KW_FN):
            return self.parse_fn(is_pub)
        elif self.check(TT.KW_STRUCT, TT.KW_DATA):
            return self.parse_struct()
        elif self.check(TT.KW_IMPL):
            return self.parse_impl()
        elif self.check(TT.KW_ENUM, TT.KW_CASES):
            return self.parse_enum()
        elif self.check(TT.KW_CONST, TT.KW_TYPE, TT.KW_TRAIT):
            self.pos += 1
            if self.check(TT.LBRACE):
                self.parse_block()
            else:
                self.advance_to_semicolon()
            return None
        elif self.check(TT.KW_EXTERN):
            self._skip_extern_block()
            return None
        else:
            # Skip unknown
            self.pos += 1
            return None

    def _skip_extern_block(self):
        self.pos += 1  # consume extern
        while not self.check(TT.EOF) and not self.check(TT.LBRACE):
            self.pos += 1
        if self.check(TT.LBRACE):
            depth = 1
            self.pos += 1
            while depth > 0 and not self.check(TT.EOF):
                if self.check(TT.LBRACE):
                    depth += 1
                elif self.check(TT.RBRACE):
                    depth -= 1
                self.pos += 1

    def advance_to_semicolon(self):
        depth = 0
        while not self.check(TT.EOF):
            if self.check(TT.LBRACE):
                depth += 1
                self.pos += 1
            elif self.check(TT.RBRACE):
                if depth == 0:
                    break
                depth -= 1
                self.pos += 1
            elif self.check(TT.SEMICOLON) and depth == 0:
                self.pos += 1
                break
            else:
                self.pos += 1

    def parse_fn(self, is_pub=False) -> FnDecl:
        self.expect(TT.KW_FN)
        name = self.expect_ident().value

        # Generic parameters: <T: Bound, ...>
        if self.check(TT.LT):
            self.pos += 1
            while not self.check(TT.GT) and not self.check(TT.EOF):
                self.parse_type()
                self.match(TT.COMMA)
            self.expect(TT.GT)

        self.expect(TT.LPAREN)
        params = []
        self_param = False

        while not self.check(TT.RPAREN) and not self.check(TT.EOF):
            # Handle 'self', 'mut self', '&self', '&mut self', '&var self'
            if self.check(TT.KW_SELF):
                self_param = True
                self.pos += 1
            elif self.check(TT.AMPERSAND):
                self.pos += 1
                if self.check(TT.KW_MUT, TT.KW_VAR):
                    self.pos += 1
                if self.check(TT.KW_SELF):
                    self_param = True
                    self.pos += 1
                else:
                    # &Type param
                    pname = self.expect_ident().value
                    self.expect(TT.COLON)
                    ptype = self.parse_type()
                    params.append((pname, ptype))
            elif self.check(TT.KW_MUT, TT.KW_VAR):
                self.pos += 1
                if self.check(TT.KW_SELF):
                    self_param = True
                    self.pos += 1
                else:
                    pname = self.expect_ident().value
                    self.expect(TT.COLON)
                    ptype = self.parse_type()
                    params.append((pname, ptype))
            else:
                pname = self.expect_ident().value
                self.expect(TT.COLON)
                ptype = self.parse_type()
                params.append((pname, ptype))

            if not self.match(TT.COMMA):
                break

        self.expect(TT.RPAREN)

        ret_type = None
        if self.match(TT.ARROW):
            ret_type = self.parse_type()

        # Handle 'where' clause
        if self.match(TT.KW_WHERE):
            while not self.check(TT.LBRACE) and not self.check(TT.EOF):
                self.pos += 1

        if self.check(TT.SEMICOLON):
            self.pos += 1
            body = []  # forward declaration: fn foo();
        else:
            body = self.parse_block()
        return FnDecl(name, params, ret_type, body, is_pub, self_param)

    def parse_type(self) -> str:
        """Parse type annotation, return as string"""
        parts = []
        self.match(TT.KW_DYN)  # Skip 'dyn'
        if self.check(TT.AMPERSAND):
            parts.append("&")
            self.pos += 1
            if self.check(TT.KW_MUT, TT.KW_VAR):
                parts.append("mut ")
                self.pos += 1

        if self.match(TT.KW_SUPER, TT.KW_CRATE, TT.KW_SELF):
            parts.append(self.tokens[self.pos - 1].value)
            if self.check(TT.COLON_COLON):
                parts.append("::")
                self.pos += 1

        if self.check(TT.KW_IMPL):
            self.pos += 1
            return "impl " + self.parse_type()

        if self.check(TT.KW_FN):
            # Function type: fn() -> Ret
            self.pos += 1
            self.expect(TT.LPAREN)
            while not self.check(TT.RPAREN) and not self.check(TT.EOF):
                self.parse_type()
                self.match(TT.COMMA)
            if not self.check(TT.EOF):
                self.expect(TT.RPAREN)
            if self.match(TT.ARROW):
                ret = self.parse_type()
                return "fn()->" + ret
            return "fn()"

        if self.check(TT.IDENT):
            name = self.pos
            type_name = self.tokens[self.pos].value
            self.pos += 1
            parts.append(type_name)
            # Trait object / function type: Fn(A, B) -> Ret
            if type_name == "Fn" and self.check(TT.LPAREN):
                self.pos += 1
                while not self.check(TT.RPAREN) and not self.check(TT.EOF):
                    self.parse_type()
                    self.match(TT.COMMA)
                if not self.check(TT.EOF):
                    self.expect(TT.RPAREN)
                if self.match(TT.ARROW):
                    ret = self.parse_type()
                    return "Fn()->" + ret
                return "Fn()"
            # Generic params
            if self.check(TT.LT):
                self.pos += 1
                type_params = []
                while not self.check(TT.GT) and not self.check(TT.EOF):
                    type_params.append(self.parse_type())
                    if not self.match(TT.COMMA):
                        break
                parts.append(f"<{', '.join(type_params)}>")
                self.expect(TT.GT)
            # Path ::
            while self.check(TT.COLON_COLON):
                self.pos += 1
                seg = self.expect_ident().value
                parts.append(f"::{seg}")

            # Generic params after path: A::B<C>
            if self.check(TT.LT):
                self.pos += 1
                type_params = []
                while not self.check(TT.GT) and not self.check(TT.EOF):
                    type_params.append(self.parse_type())
                    if not self.match(TT.COMMA):
                        break
                parts.append(f"<{', '.join(type_params)}>")
                self.expect(TT.GT)

            # Simple trait bounds: T: Clone
            if self.match(TT.COLON):
                parts.append(":")
                parts.append(self.parse_type())
        elif self.check(TT.LBRACKET):
            # Array type: [Type] or [Type; N]
            self.pos += 1
            inner = self.parse_type()
            if self.match(TT.SEMICOLON):
                self.parse_expr()  # skip N
            self.expect(TT.RBRACKET)
            parts.append(f"[{inner}]")
        elif self.check(TT.LPAREN):
            # Tuple type
            self.pos += 1
            inner_types = []
            while not self.check(TT.RPAREN) and not self.check(TT.EOF):
                inner_types.append(self.parse_type())
                if not self.match(TT.COMMA):
                    break
            self.expect(TT.RPAREN)
            parts.append(f"({', '.join(inner_types)})")
        else:
            parts.append("i64")  # fallback

        return "".join(parts)

    def parse_struct(self) -> StructDecl:
        self.match(TT.KW_STRUCT, TT.KW_DATA)
        name = self.expect_ident().value
        # Handle generics <T>
        if self.check(TT.LT):
            self.pos += 1
            while not self.check(TT.GT) and not self.check(TT.EOF):
                self.parse_type()
                self.match(TT.COMMA)
            self.expect(TT.GT)

        # Skip "derives Trait" or "derives [Trait, ...]"
        if self.check(TT.IDENT) and self.current().value == "derives":
            self.pos += 1
            if self.check(TT.LBRACKET):
                depth = 1
                self.pos += 1
                while depth > 0 and not self.check(TT.EOF):
                    if self.check(TT.LBRACKET):
                        depth += 1
                    elif self.check(TT.RBRACKET):
                        depth -= 1
                    self.pos += 1
            else:
                self.parse_type()

        self.skip_where_clause()
        self.expect(TT.LBRACE)
        fields = []
        while not self.check(TT.RBRACE) and not self.check(TT.EOF):
            self.match(TT.KW_OPEN, TT.KW_PUB)
            fname = self.expect_ident().value
            self.expect(TT.COLON)
            ftype = self.parse_type()
            fields.append((fname, ftype))
            self.match(TT.COMMA)
        if not self.check(TT.EOF):
            self.expect(TT.RBRACE)
        return StructDecl(name, fields)

    def parse_impl(self) -> ImplBlock:
        self.expect(TT.KW_IMPL)

        # Handle impl<T>
        if self.check(TT.LT):
            self.pos += 1
            while not self.check(TT.GT) and not self.check(TT.EOF):
                self.parse_type()
                self.match(TT.COMMA)
            self.expect(TT.GT)

        name = self.expect_ident().value
        # Handle Path::To::Type
        while self.check(TT.COLON_COLON):
            self.pos += 1
            name += "::" + self.expect_ident().value

        # Handle generics in type: impl Trait for Struct<T>
        if self.check(TT.LT):
            self.pos += 1
            while not self.check(TT.GT) and not self.check(TT.EOF):
                self.parse_type()
                self.match(TT.COMMA)
            self.expect(TT.GT)

        if self.match(TT.KW_FOR):
            # impl Trait for Type
            name += " for " + self.parse_type()

        self.skip_where_clause()
        self.expect(TT.LBRACE)
        methods = []
        while not self.check(TT.RBRACE) and not self.check(TT.EOF):
            is_pub = bool(self.match(TT.KW_PUB))
            if self.check(TT.KW_FN):
                methods.append(self.parse_fn(is_pub))
            else:
                self.pos += 1  # skip unknown
        if not self.check(TT.EOF):
            self.expect(TT.RBRACE)
        return ImplBlock(name, methods)

    def parse_enum(self) -> EnumDecl:
        self.match(TT.KW_ENUM, TT.KW_CASES)
        name = self.expect_ident().value
        # Skip generics: enum Name<T> { or cases Name<T> {
        if self.check(TT.LT):
            depth = 1
            self.pos += 1
            while depth > 0 and not self.check(TT.EOF):
                if self.check(TT.LT):
                    depth += 1
                elif self.check(TT.GT):
                    depth -= 1
                self.pos += 1
        # Skip "derives [Trait, ...]" before {
        if self.check(TT.IDENT) and self.current().value == "derives":
            self.pos += 1
            if self.check(TT.LBRACKET):
                depth = 1
                self.pos += 1
                while depth > 0 and not self.check(TT.EOF):
                    if self.check(TT.LBRACKET):
                        depth += 1
                    elif self.check(TT.RBRACKET):
                        depth -= 1
                    self.pos += 1
        self.expect(TT.LBRACE)
        variants = []
        while not self.check(TT.RBRACE) and not self.check(TT.EOF):
            if self.check(TT.IDENT):
                v = self.tokens[self.pos].value
                self.pos += 1
                variants.append(v)
                # Skip tuple variants (...)
                if self.check(TT.LPAREN):
                    depth = 1
                    self.pos += 1
                    while depth > 0 and not self.check(TT.EOF):
                        if self.check(TT.LPAREN):
                            depth += 1
                        elif self.check(TT.RPAREN):
                            depth -= 1
                        self.pos += 1
                self.match(TT.COMMA)
            else:
                self.pos += 1
        if not self.check(TT.EOF):
            self.expect(TT.RBRACE)
        return EnumDecl(name, variants)

    def parse_block(self) -> List[ASTNode]:
        self.expect(TT.LBRACE)
        stmts = []
        while not self.check(TT.RBRACE) and not self.check(TT.EOF):
            stmt = self.parse_statement()
            if stmt:
                stmts.append(stmt)
        if not self.check(TT.EOF):
            self.expect(TT.RBRACE)
        return stmts

    def parse_statement(self) -> Optional[ASTNode]:
        if self.check(TT.KW_LET, TT.KW_VAR):
            return self.parse_let()
        elif self.check(TT.KW_RETURN, TT.KW_YIELD):
            return self.parse_return()
        elif self.check(TT.KW_IF, TT.KW_CHECK):
            return self.parse_if()

        elif self.check(TT.KW_WHILE):
            return self.parse_while()
        elif self.check(TT.KW_FOR):
            return self.parse_for()
        elif self.check(TT.KW_BREAK):
            self.pos += 1
            self.match(TT.SEMICOLON)
            return BreakStmt()
        elif self.check(TT.KW_CONTINUE):
            self.pos += 1
            self.match(TT.SEMICOLON)
            return ContinueStmt()

        # Nova v10 keywords and constructs
        elif self.check(TT.KW_EACH):
            # each var in iter { ... }
            return self.parse_for()

        elif self.check(TT.KW_FLOW, TT.KW_SPAWN, TT.KW_ON):
            # flow { ... }, spawn { ... }, on event { ... }
            self.pos += 1
            if self.check(TT.IDENT):
                self.pos += 1  # skip event name for 'on'
            if self.check(TT.LBRACE):
                self.parse_block()
            return None  # Skip/Ignore for bootstrap

        elif self.check(TT.KW_ALIAS, TT.KW_RULES):
            # alias NewType = OldType; or rules { ... }
            self.pos += 1
            self.advance_to_semicolon()
            return None
        elif self.check(TT.RBRACE):
            return None
        else:
            expr = self.parse_expr()
            self.match(TT.SEMICOLON)
            return ExprStmt(expr)

    def parse_let(self) -> LetStmt:
        token = self.match(TT.KW_LET, TT.KW_VAR)
        is_mut = (token.type == TT.KW_VAR) or bool(self.match(TT.KW_MUT))

        if self.match(TT.LPAREN):
            # Tuple pattern
            names = []
            while not self.check(TT.RPAREN) and not self.check(TT.EOF):
                names.append(self.expect_ident().value)
                self.match(TT.COMMA)
            self.expect(TT.RPAREN)
            name = names
        else:
            name = self.expect_ident().value

        type_ann = None
        if self.match(TT.COLON):
            type_ann = self.parse_type()
        value = None
        if self.match(TT.EQ):
            value = self.parse_expr()
        self.match(TT.SEMICOLON)
        return LetStmt(name, type_ann, value, is_mut)

    def parse_return(self) -> ReturnStmt:
        self.match(TT.KW_RETURN, TT.KW_YIELD)
        if self.check(TT.SEMICOLON) or self.check(TT.RBRACE):
            self.match(TT.SEMICOLON)
            return ReturnStmt(None)
        val = self.parse_expr()
        self.match(TT.SEMICOLON)
        return ReturnStmt(val)

    def parse_if(self) -> IfStmt:
        self.match(TT.KW_IF, TT.KW_CHECK)
        if self.check(TT.KW_LET):
            # check let Pattern = expr { block } — skip until {
            self.pos += 1
            depth = 0
            while not self.check(TT.EOF):
                t = self.current().type
                if t in (TT.LPAREN, TT.LBRACKET, TT.LBRACE):
                    depth += 1
                elif t in (TT.RPAREN, TT.RBRACKET, TT.RBRACE):
                    depth -= 1
                elif t == TT.LBRACE and depth == 0:
                    break
                self.pos += 1
            cond = IntLit(0)  # bootstrap: condition discarded
            if self.check(TT.EOF):
                then_b = []  # no block at EOF
            else:
                then_b = self.parse_block()
        else:
            cond = self.parse_expr()
            then_b = self.parse_block()
        else_b = None
        if self.match(TT.KW_ELSE):
            if self.check(TT.KW_IF, TT.KW_CHECK):
                else_b = [self.parse_if()]
            else:
                else_b = self.parse_block()
        return IfStmt(cond, then_b, else_b)

    def parse_while(self) -> WhileStmt:
        self.expect(TT.KW_WHILE)
        cond = self.parse_expr()
        body = self.parse_block()
        return WhileStmt(cond, body)

    def parse_for(self) -> ForStmt:
        self.match(TT.KW_FOR, TT.KW_EACH)
        if self.match(TT.LPAREN):
            # each (a, b) in expr { } — tuple destructuring; bootstrap uses first name
            names = []
            while not self.check(TT.RPAREN) and not self.check(TT.EOF):
                names.append(self.expect_ident().value)
                if not self.match(TT.COMMA):
                    break
            self.expect(TT.RPAREN)
            var = names[0] if names else "_"
        else:
            var = self.expect_ident().value
        self.expect(TT.KW_IN)
        iter_expr = self.parse_expr()
        body = self.parse_block()
        return ForStmt(var, iter_expr, body)

    # -----------------------------------------------------------------------
    # EXPRESSION PARSING (Pratt parser style)
    # -----------------------------------------------------------------------

    def parse_expr(self) -> ASTNode:
        return self.parse_assignment()

    def parse_assignment(self) -> ASTNode:
        left = self.parse_or()

        # Handle 'as' casting
        while self.match(TT.KW_AS):
            target_type = self.parse_type()
            left = Call(Identifier("_as"), [left, StrLit(target_type)])

        ASSIGN_OPS = {TT.EQ, TT.PLUS_EQ, TT.MINUS_EQ, TT.STAR_EQ, TT.SLASH_EQ}
        if self.current().type in ASSIGN_OPS:
            op = self.tokens[self.pos].value
            self.pos += 1
            right = self.parse_assignment()
            return Assign(left, op, right)
        return left

    def parse_or(self) -> ASTNode:
        left = self.parse_and()
        while self.check(TT.OR_OR):
            op = "||"
            self.pos += 1
            right = self.parse_and()
            left = BinOp(op, left, right)
        return left

    def parse_and(self) -> ASTNode:
        left = self.parse_eq()
        while self.check(TT.AND_AND):
            self.pos += 1
            right = self.parse_eq()
            left = BinOp("&&", left, right)
        return left

    def parse_eq(self) -> ASTNode:
        left = self.parse_cmp()
        while self.check(TT.EQ_EQ, TT.BANG_EQ):
            op = self.tokens[self.pos].value
            self.pos += 1
            right = self.parse_cmp()
            left = BinOp(op, left, right)
        return left

    def parse_cmp(self) -> ASTNode:
        left = self.parse_add()
        while self.check(TT.LT, TT.GT, TT.LT_EQ, TT.GT_EQ):
            op = self.tokens[self.pos].value
            self.pos += 1
            right = self.parse_add()
            left = BinOp(op, left, right)
        return left

    def parse_add(self) -> ASTNode:
        left = self.parse_mul()
        while self.check(TT.PLUS, TT.MINUS):
            op = self.tokens[self.pos].value
            self.pos += 1
            right = self.parse_mul()
            left = BinOp(op, left, right)
        return left

    def parse_mul(self) -> ASTNode:
        left = self.parse_unary()
        while self.check(TT.STAR, TT.SLASH, TT.PERCENT):
            op = self.tokens[self.pos].value
            self.pos += 1
            right = self.parse_unary()
            left = BinOp(op, left, right)
        return left

    def parse_unary(self) -> ASTNode:
        if self.match(TT.BANG):
            return UnaryOp("!", self.parse_unary())
        if self.match(TT.MINUS):
            return UnaryOp("-", self.parse_unary())
        if self.match(TT.AMPERSAND):
            self.match(TT.KW_VAR, TT.KW_MUT)  # optional: &var self / &mut self
            return UnaryOp("&", self.parse_unary())
        if self.match(TT.STAR):
            return UnaryOp("*", self.parse_unary())  # dereference *expr
        return self.parse_postfix()

    def parse_postfix(self) -> ASTNode:
        expr = self.parse_primary()
        while True:
            if self.check(TT.DOT):
                self.pos += 1
                field = self.expect_ident().value
                if self.check(TT.LPAREN):
                    args = self.parse_args()
                    expr = MethodCall(expr, field, args)
                else:
                    expr = FieldAccess(expr, field)
            elif self.check(TT.LBRACKET):
                self.pos += 1
                if self.check(TT.DOT_DOT):
                    self.pos += 1
                    end = self.parse_expr() if not self.check(TT.RBRACKET) else None
                    idx = RangeLit(None, end)
                else:
                    if self.check(TT.RBRACKET):
                        idx = IntLit(0)  # empty index arr[]
                    else:
                        idx = self.parse_expr()
                        if self.check(TT.DOT_DOT):
                            self.pos += 1
                            end = self.parse_expr() if not self.check(TT.RBRACKET) else None
                            idx = RangeLit(idx, end)
                self.expect(TT.RBRACKET)
                expr = Index(expr, idx)
            elif self.check(TT.LPAREN):
                args = self.parse_args()
                expr = Call(expr, args)
            elif self.check(TT.DOT_DOT):
                # Range: start..end
                self.pos += 1
                end = self.parse_expr() if not self.check(TT.RBRACKET) and not self.check(TT.RBRACE) else None
                expr = RangeLit(expr, end)
            elif self.check(TT.COLON_COLON):
                self.pos += 1
                if self.check(TT.LT):
                    # Turbofish: expr::<Type>() — skip type arg for bootstrap
                    self.pos += 1
                    self.parse_type()
                    self.expect(TT.GT)
                    if self.check(TT.LPAREN):
                        args = self.parse_args()
                        expr = Call(expr, args)
                else:
                    name = self.expect_ident().value
                    if self.check(TT.LPAREN):
                        args = self.parse_args()
                        expr = Call(FieldAccess(expr, name), args)
                    else:
                        expr = FieldAccess(expr, name)
            elif self.check(TT.QUESTION):
                self.pos += 1
                expr = UnaryOp("?", expr)
            elif self.check(TT.LBRACE):
                # Struct literal: Type { fields } or Module::Type { fields }
                is_struct = False
                if isinstance(expr, Identifier) and expr.name[0:1].isupper():
                    is_struct = True
                elif isinstance(expr, FieldAccess) and not self._expr_is_self_like(expr):
                    last = self._expr_path_last_segment(expr)
                    if last and last[0:1].isupper():
                        is_struct = True
                
                if is_struct:
                    # Disambiguate from block by looking ahead
                    next_tok = self.tokens[self.pos + 1].type
                    if next_tok == TT.RBRACE:
                        pass # Empty struct
                    elif next_tok == TT.IDENT:
                        tok2 = self.tokens[self.pos + 2].type
                        if tok2 not in (TT.COLON, TT.COMMA, TT.RBRACE):
                            is_struct = False
                    else:
                        is_struct = False
                        
                if not is_struct:
                    break
                self.pos += 1
                fields = []
                while not self.check(TT.RBRACE) and not self.check(TT.EOF):
                    fname = self.expect_ident().value
                    if self.match(TT.COLON):
                        fval = self.parse_expr()
                    else:
                        fval = Identifier(fname)
                    fields.append((fname, fval))
                    if not self.match(TT.COMMA):
                        break
                self.expect(TT.RBRACE)
                path_str = self._expr_to_path_str(expr)
                expr = StructLit(path_str, fields)
            else:
                break
        return expr

    def _expr_is_self_like(self, expr: ASTNode) -> bool:
        if isinstance(expr, Identifier):
            return expr.name in ("_self", "self")
        if isinstance(expr, FieldAccess):
            return self._expr_is_self_like(expr.obj)
        return False

    def _expr_path_first_segment(self, expr: ASTNode) -> str:
        if isinstance(expr, Identifier):
            return expr.name
        if isinstance(expr, FieldAccess):
            return self._expr_path_first_segment(expr.obj)
        return ""

    def _expr_path_last_segment(self, expr: ASTNode) -> str:
        if isinstance(expr, Identifier):
            return expr.name
        if isinstance(expr, FieldAccess):
            return expr.field
        return ""


    def _expr_to_path_str(self, expr: ASTNode) -> str:
        if isinstance(expr, Identifier):
            return expr.name
        if isinstance(expr, FieldAccess):
            return self._expr_to_path_str(expr.obj) + "::" + expr.field
        return "Struct"

    def parse_args(self) -> List[ASTNode]:
        self.expect(TT.LPAREN)
        args = []
        while not self.check(TT.RPAREN) and not self.check(TT.EOF):
            args.append(self.parse_expr())
            if not self.match(TT.COMMA):
                break
        if not self.check(TT.EOF):
            self.expect(TT.RPAREN)
        return args

    def parse_primary(self) -> ASTNode:
        t = self.current()

        if t.type == TT.INT_LIT:
            self.pos += 1
            return IntLit(t.value)
        elif t.type == TT.FLOAT_LIT:
            self.pos += 1
            return FloatLit(t.value)
        elif t.type == TT.STR_LIT:
            self.pos += 1
            return StrLit(t.value)
        elif t.type == TT.BOOL_LIT:
            self.pos += 1
            return BoolLit(t.value)
        elif t.type == TT.KW_TRUE:
            self.pos += 1
            return BoolLit(True)
        elif t.type == TT.KW_FALSE:
            self.pos += 1
            return BoolLit(False)

        elif t.type == TT.KW_SELF:
            self.pos += 1
            return Identifier("_self")

        elif t.type == TT.IDENT:
            self.pos += 1
            name = t.value

            # format!() macro
            if name == "format" and self.check(TT.BANG):
                self.pos += 1
                return self.parse_format_macro()

            # println! / print! / eprintln!
            if name in ("println", "print", "eprintln", "eprint") and self.check(
                TT.BANG
            ):
                self.pos += 1
                return self.parse_print_macro(name)

            # vec![]
            if name == "vec" and self.check(TT.BANG):
                self.pos += 1
                return self.parse_vec_macro()

            # Generic macro: ident!(args...)
            if self.check(TT.BANG):
                self.pos += 1
                self.expect(TT.LPAREN)
                args = []
                while not self.check(TT.RPAREN) and not self.check(TT.EOF):
                    args.append(self.parse_expr())
                    if not self.match(TT.COMMA):
                        break
                self.expect(TT.RPAREN)
                return Call(Identifier(name), args)

            # Struct literal: Name { field: val, ... }
            if self.check(TT.LBRACE) and self.peek(-1).type == TT.IDENT:
                # Only if followed by ident: expr pattern
                saved = self.pos
                try:
                    return self.parse_struct_literal(name)
                except:
                    self.pos = saved

            return Identifier(name)

        elif t.type == TT.LPAREN:
            self.pos += 1
            if self.check(TT.RPAREN):
                self.pos += 1
                return IntLit(0)  # unit ()
            expr = self.parse_expr()
            self.expect(TT.RPAREN)
            return expr

        elif t.type == TT.LBRACKET:
            # Array literal
            self.pos += 1
            items = []
            while not self.check(TT.RBRACKET) and not self.check(TT.EOF):
                items.append(self.parse_expr())
                if not self.match(TT.COMMA):
                    break
            self.expect(TT.RBRACKET)
            return Call(Identifier("_array"), items)

        elif t.type == TT.LBRACE:
            # Block expression
            stmts = self.parse_block()
            return Block(stmts)

        elif t.type == TT.PIPE:
            return self.parse_closure()

        elif t.type in (TT.KW_MATCH, TT.KW_GIVEN):
            # match expr { pattern => expr, ... }
            self.pos += 1
            cond = self.parse_expr()
            self.expect(TT.LBRACE)
            depth = 1
            while depth > 0 and not self.check(TT.EOF):
                if self.check(TT.LBRACE):
                    depth += 1
                elif self.check(TT.RBRACE):
                    depth -= 1
                self.pos += 1
            return Call(Identifier("match"), [cond])

        elif t.type == TT.KW_ABORT:
            self.pos += 1
            return Call(Identifier("abort"), [])

        elif t.type == TT.KW_UNSAFE:
            self.pos += 1
            return self.parse_primary()  # recurse to handle the block

        elif t.type == TT.RBRACKET:
            self.error("Unexpected ']'")
        else:
            # Skip and return a null literal
            self.pos += 1
            return IntLit(0)

    def parse_closure(self) -> ASTNode:
        """|args| body"""
        self.expect(TT.PIPE)
        params = []
        while not self.check(TT.PIPE) and not self.check(TT.EOF):
            pname = self.expect_ident().value
            params.append(pname)
            self.match(TT.COMMA)
        self.expect(TT.PIPE)

        if self.check(TT.LBRACE):
            body = self.parse_block()
        else:
            body = [self.parse_expr()]
        return Call(Identifier("_closure"), [StrLit(",".join(params)), Block(body)])

    def parse_format_macro(self) -> ASTNode:
        """format!("template", args...)"""
        self.expect(TT.LPAREN)
        template = ""
        if self.check(TT.STR_LIT):
            template = self.tokens[self.pos].value
            self.pos += 1
        args = []
        while self.match(TT.COMMA):
            if self.check(TT.RPAREN):
                break
            args.append(self.parse_expr())
        self.expect(TT.RPAREN)
        return FormatStr(template, args)

    def parse_print_macro(self, fn_name: str) -> ASTNode:
        """println!(...) -> Call to runtime function"""
        self.expect(TT.LPAREN)
        if self.check(TT.RPAREN):
            self.pos += 1
            return Call(Identifier(fn_name), [StrLit("")])

        template = ""
        if self.check(TT.STR_LIT):
            template = self.tokens[self.pos].value
            self.pos += 1

        args = []
        while self.match(TT.COMMA):
            if self.check(TT.RPAREN):
                break
            args.append(self.parse_expr())
        self.expect(TT.RPAREN)

        if args:
            fmt_str = FormatStr(template, args)
        else:
            fmt_str = StrLit(template)
        return Call(Identifier(fn_name), [fmt_str])

    def parse_vec_macro(self) -> ASTNode:
        """vec![...] -> array"""
        if self.check(TT.LBRACKET):
            self.pos += 1
            items = []
            while not self.check(TT.RBRACKET) and not self.check(TT.EOF):
                items.append(self.parse_expr())
                if not self.match(TT.COMMA):
                    break
            self.expect(TT.RBRACKET)
            return Call(Identifier("_vec"), items)
        return Call(Identifier("_vec"), [])

    def parse_struct_literal(self, name: str) -> StructLit:
        self.expect(TT.LBRACE)
        fields = []
        while not self.check(TT.RBRACE) and not self.check(TT.EOF):
            fname = self.expect_ident().value
            if self.match(TT.COLON):
                fval = self.parse_expr()
            else:
                fval = Identifier(fname)  # shorthand: field only
            fields.append((fname, fval))
            if not self.match(TT.COMMA):
                break
        self.expect(TT.RBRACE)
        return StructLit(name, fields)


# ===========================================================================
# C CODE GENERATOR
# ===========================================================================

LN_TYPE_TO_C = {
    "int": "int64_t",
    "i8": "int8_t",
    "i16": "int16_t",
    "i32": "int32_t",
    "i64": "int64_t",
    "u8": "uint8_t",
    "u16": "uint16_t",
    "u32": "uint32_t",
    "u64": "uint64_t",
    "f32": "float",
    "f64": "double",
    "bool": "int",
    "str": "const char*",
    "String": "char*",
    "tensor": "nova_tensor_t*",
    "tape": "nova_grad_tape_t*",
    "void": "void",
    "char": "char",
}


class CodeGen:
    def __init__(self):
        self.output: List[str] = []
        self.indent_level = 0
        self.structs: Dict[str, StructDecl] = {}
        self.enums: Dict[str, EnumDecl] = {}
        self.functions: Dict[str, FnDecl] = {}
        self.temp_count = 0
        self.string_pool: Dict[str, str] = {}  # value -> var name
        self.string_decls: List[str] = []

    def safe_name(self, name: str) -> str:
        reserved = {
            "auto",
            "break",
            "case",
            "char",
            "const",
            "continue",
            "default",
            "do",
            "double",
            "else",
            "enum",
            "extern",
            "float",
            "for",
            "goto",
            "if",
            "inline",
            "int",
            "long",
            "register",
            "restrict",
            "return",
            "short",
            "signed",
            "sizeof",
            "static",
            "struct",
            "switch",
            "typedef",
            "union",
            "unsigned",
            "void",
            "volatile",
            "while",
            "bool",
            "true",
            "false",
            "new",
            "delete",
            "class",
            "namespace",
            "template",
            "typename",
            "friend",
            "virtual",
            "operator",
            "explicit",
            "mutable",
            "main",
        }
        if name in reserved:
            return f"_{name}"
        return name

    def emit(self, line: str):
        self.output.append("    " * self.indent_level + line)

    def emit_raw(self, line: str):
        self.output.append(line)

    def fresh_tmp(self) -> str:
        self.temp_count += 1
        return f"_t{self.temp_count}"

    def type_to_c(self, t: str) -> str:
        if t in LN_TYPE_TO_C:
            return LN_TYPE_TO_C[t]
        if t.startswith("&"):
            inner = t[1:]
            if inner.startswith("mut "):
                inner = inner[4:]
            return self.type_to_c(inner) + "*"
        if t.startswith("Vec<") or t.startswith("Vec::"):
            return "ln_vec_t*"
        if t.startswith("Option<"):
            return "void*"
        if t.startswith("Result<"):
            return "ln_result_t"
        # Struct type
        return f"struct {t}*"

    def generate(self, program: Program) -> str:
        # Pre-pass: collect struct and function names
        for item in program.items:
            if isinstance(item, StructDecl):
                self.structs[item.name] = item
            elif isinstance(item, EnumDecl):
                self.enums[item.name] = item
            elif isinstance(item, FnDecl):
                self.functions[item.name] = item
            elif isinstance(item, ImplBlock):
                for m in item.methods:
                    self.functions[f"{item.type_name}__{m.name}"] = m

        header = self.gen_header()
        body_parts = []

        # Enum definitions
        enum_defs = []
        for e in self.enums.values():
            enum_defs.append(self.gen_enum(e))
        if enum_defs:
            body_parts.append("\n".join(enum_defs))

        # Struct declarations (forward)
        forward_decls = []
        for s in self.structs.values():
            forward_decls.append(f"struct {s.name};")
        if forward_decls:
            body_parts.append("\n".join(forward_decls))

        # Struct definitions
        struct_defs = []
        for s in self.structs.values():
            struct_defs.append(self.gen_struct(s))
        if struct_defs:
            body_parts.append("\n".join(struct_defs))

        # Function forward declarations
        fn_forwards = []
        for item in program.items:
            if isinstance(item, FnDecl):
                fn_forwards.append(self.gen_fn_forward(item))
            elif isinstance(item, ImplBlock):
                for m in item.methods:
                    fn_forwards.append(self.gen_method_forward(item.type_name, m))
        if fn_forwards:
            body_parts.append("\n".join(fn_forwards))

        # Function definitions
        fn_defs = []
        for item in program.items:
            if isinstance(item, FnDecl):
                fn_defs.append(self.gen_fn(item))
            elif isinstance(item, ImplBlock):
                for m in item.methods:
                    fn_defs.append(self.gen_method(item.type_name, m))
        if fn_defs:
            body_parts.append("\n".join(fn_defs))

        all_parts = [header] + body_parts
        code = "\n\n".join(all_parts)

        # Inject string pool after header
        if self.string_decls:
            string_section = "\n".join(self.string_decls)
            code = (
                header + "\n\n" + string_section + "\n\n" + "\n\n".join(body_parts[1:])
            )

        return code

    def gen_header(self) -> str:
        return """\
/* Generated by Nova Stage 0 Compiler */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* ---- Nova Runtime Types ---- */
typedef void* tape;
typedef void* tensor;
typedef struct { char* data; int64_t len; int64_t cap; } ln_string_t;
typedef struct { void** data; int64_t len; int64_t cap; } ln_vec_t;
typedef struct { int ok; void* value; char* err; } ln_result_t;

/* ---- Runtime helpers ---- */
static char _fmt_buf[4096];

static char* ln_format_int(int64_t v) {
    char* buf = malloc(32);
    snprintf(buf, 32, "%lld", (long long)v);
    return buf;
}
static char* ln_format_float(double v) {
    char* buf = malloc(64);
    snprintf(buf, 64, "%g", v);
    return buf;
}
static int64_t ln_str_len(const char* s) { return s ? (int64_t)strlen(s) : 0; }
static char* ln_str_concat(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t la = strlen(a), lb = strlen(b);
    char* r = malloc(la+lb+1);
    memcpy(r, a, la);
    memcpy(r+la, b, lb);
    r[la+lb] = 0;
    return r;
}

/* ---- Runtime println/print ---- */
static void println(const char* s) { printf("%s\\n", s ? s : ""); }
static void print_ln(const char* s) { printf("%s", s ? s : ""); }
static void eprintln_fn(const char* s) { fprintf(stderr, "%s\\n", s ? s : ""); }

/* ---- External runtime functions ---- */
extern void nova_runtime_println(const char* ptr, int64_t len);
extern void nova_runtime_print(const char* ptr, int64_t len);
extern void nova_runtime_print_int(int64_t value);
extern void nova_runtime_print_float(double value);
extern char* nova_runtime_read_line(void);
extern int64_t nova_net_socket_create(int64_t domain, int64_t type, int64_t protocol);
extern int64_t nova_net_socket_bind(int64_t fd, const char* host, int64_t port);
extern int64_t nova_net_socket_listen(int64_t fd, int64_t backlog);
extern int64_t nova_net_socket_accept(int64_t fd);
extern int64_t nova_net_socket_connect(int64_t fd, const char* host, int64_t port);
extern int64_t nova_net_socket_read(int64_t fd, char* buf, int64_t len);
extern int64_t nova_net_socket_write(int64_t fd, const char* buf, int64_t len);
extern void nova_net_socket_close(int64_t fd);
extern char* nova_net_get_request_path(int64_t client_fd);
extern char* nova_net_get_query_param(const char* url, const char* param_name);
extern void nova_net_send_http_response(int64_t client_fd, int64_t status_code, const char* content_type, const char* body);
extern int64_t init_linguist(const char* dict_path);
extern const char* lookup_linguist(const char* word);
extern const char* lookup_linguist_category(const char* word);
extern const char* analyze_morphology(const char* word);
extern const char* read_console_input(void);
extern const char* advance_pointer(const char* ptr, int64_t offset);
extern const char* get_app_mode_env(void);
extern const char* get_http_body(const char* response);



/* ---- AST Runtime ---- */
extern int64_t ast_node_new(int64_t kind, int64_t token_val, int64_t left_ptr, int64_t right_ptr);
extern int64_t ast_node_get_kind(int64_t ptr);
extern int64_t ast_node_get_left(int64_t ptr);
extern int64_t ast_node_get_right(int64_t ptr);
extern void ast_node_print(int64_t ptr, int64_t depth);

/* ---- Runtime Extensions ---- */
extern char* fs__read_to_string(const char* path);
extern int64_t fs__write(const char* path, const char* content);
extern int64_t nova_system(const char* command);
extern int64_t nova_runtime_char_at(const char* s, int64_t pos);
extern int64_t nova_runtime_str_len(const char* s);

/* ---- AI Sovereign Engine ---- */
extern tape  nova_grad_tape_create(void);
extern void  nova_grad_tape_begin(tape t);
extern void  nova_grad_tape_watch(tape t, tensor x);
extern int   nova_grad_tape_backward(tape t, tensor loss);
extern tensor nova_tensor_create(const int64_t* shape, int64_t ndim, int dtype);
"""

    def gen_enum(self, e: EnumDecl) -> str:
        lines = []
        for i, v in enumerate(e.variants):
            lines.append(f"#define NOVA_{e.name.upper()}_{v.upper()} {i}")
        return "\n".join(lines)

    def gen_struct(self, s: StructDecl) -> str:
        lines = [f"struct {s.name} {{"]
        for fname, ftype in s.fields:
            ctype = self.type_to_c(ftype)
            lines.append(f"    {ctype} {self.safe_name(fname)};")
        lines.append("};")
        return "\n".join(lines)

    def gen_fn_sig(self, fn: FnDecl) -> str:
        ret = self.type_to_c(fn.ret_type) if fn.ret_type else "void"
        if fn.name == "main":
            ret = "int"
        params = []
        if fn.self_param:
            params.append("void* _self")
        for pname, ptype in fn.params:
            params.append(f"{self.type_to_c(ptype)} {self.safe_name(pname)}")
        return f'{ret} {fn.name}({", ".join(params) if params else "void"})'

    def gen_method_sig(self, struct_name: str, fn: FnDecl) -> str:
        ret = self.type_to_c(fn.ret_type) if fn.ret_type else "void"
        params = []
        if fn.self_param:
            params.append(f"struct {struct_name}* _self")
        for pname, ptype in fn.params:
            params.append(f"{self.type_to_c(ptype)} {self.safe_name(pname)}")
        return (
            f'{ret} {struct_name}__{fn.name}({", ".join(params) if params else "void"})'
        )

    def gen_fn_forward(self, fn: FnDecl) -> str:
        return self.gen_fn_sig(fn) + ";"

    def gen_method_forward(self, struct_name: str, fn: FnDecl) -> str:
        return self.gen_method_sig(struct_name, fn) + ";"

    def gen_fn(self, fn: FnDecl) -> str:
        lines = [self.gen_fn_sig(fn) + " {"]
        saved_output = self.output
        saved_indent = self.indent_level
        self.output = []
        self.indent_level = 1
        for stmt in fn.body:
            self.gen_stmt(stmt)
        # Add return 0 to main if not already there
        if fn.name == "main":
            self.output.append("    return 0;")
        for line in self.output:
            lines.append(line)
        lines.append("}")
        self.output = saved_output
        self.indent_level = saved_indent
        return "\n".join(lines)

    def gen_method(self, struct_name: str, fn: FnDecl) -> str:
        lines = [self.gen_method_sig(struct_name, fn) + " {"]
        saved_output = self.output
        saved_indent = self.indent_level
        self.output = []
        self.indent_level = 1
        for stmt in fn.body:
            self.gen_stmt(stmt)
        for line in self.output:
            lines.append(line)
        lines.append("}")
        self.output = saved_output
        self.indent_level = saved_indent
        return "\n".join(lines)

    def gen_stmt(self, node: ASTNode):
        if isinstance(node, LetStmt):
            if isinstance(node.name, list):
                # Tuple pattern
                val = self.gen_expr(node.value)
                tmp = self.fresh_tmp()
                ctype = "void*"
                self.emit(f"{ctype} {tmp} = (void*){val};")
                for i, name in enumerate(node.name):
                    self.emit(
                        f"int64_t {self.safe_name(name)} = ((int64_t*){tmp})[{i}];"
                    )
            else:
                ctype = self.type_to_c(node.type_ann) if node.type_ann else "int64_t"
                if node.value:
                    val = self.gen_expr(node.value)
                    # Auto-detect type from value if unknown
                    if not node.type_ann:
                        ctype = self.infer_c_type(node.value)
                    self.emit(f"{ctype} {self.safe_name(node.name)} = {val};")
                else:
                    self.emit(f"{ctype} {self.safe_name(node.name)} = 0;")

        elif isinstance(node, ReturnStmt):
            if node.value:
                val = self.gen_expr(node.value)
                self.emit(f"return {val};")
            else:
                self.emit("return;")

        elif isinstance(node, ExprStmt):
            expr = self.gen_expr(node.expr)
            self.emit(f"{expr};")

        elif isinstance(node, IfStmt):
            cond = self.gen_expr(node.cond)
            self.emit(f"if ({cond}) {{")
            self.indent_level += 1
            for s in node.then_block:
                self.gen_stmt(s)
            self.indent_level -= 1
            if node.else_block:
                self.emit("} else {")
                self.indent_level += 1
                for s in node.else_block:
                    self.gen_stmt(s)
                self.indent_level -= 1
            self.emit("}")

        elif isinstance(node, WhileStmt):
            cond = self.gen_expr(node.cond)
            self.emit(f"while ({cond}) {{")
            self.indent_level += 1
            for s in node.body:
                self.gen_stmt(s)
            self.indent_level -= 1
            self.emit("}")

        elif isinstance(node, ForStmt):
            # for var in start..end
            if isinstance(node.iter, RangeLit):
                start = self.gen_expr(node.iter.start)
                end = self.gen_expr(node.iter.end)
                self.emit(
                    f"for (int64_t {self.safe_name(node.var)} = {start}; {self.safe_name(node.var)} < {end}; {self.safe_name(node.var)}++) {{"
                )
            else:
                # Generic iteration (treat as array)
                iter_e = self.gen_expr(node.iter)
                self.emit(
                    f"/* for {self.safe_name(node.var)} in {iter_e} - simplified */"
                )
                self.emit(f"for (int64_t _i = 0; _i < 0; _i++) {{")
                self.emit(
                    f"    int64_t {self.safe_name(node.var)} = 0; /* TODO: array iteration */"
                )
            self.indent_level += 1
            for s in node.body:
                self.gen_stmt(s)
            self.indent_level -= 1
            self.emit("}")

        elif isinstance(node, BreakStmt):
            self.emit("break;")

        elif isinstance(node, ContinueStmt):
            self.emit("continue;")

        elif isinstance(node, Block):
            self.emit("{")
            self.indent_level += 1
            for s in node.stmts:
                self.gen_stmt(s)
            self.indent_level -= 1
            self.emit("}")

    def infer_c_type(self, node: ASTNode) -> str:
        if isinstance(node, IntLit):
            return "int64_t"
        if isinstance(node, FloatLit):
            return "double"
        if isinstance(node, StrLit):
            return "const char*"
        if isinstance(node, BoolLit):
            return "int"
        if isinstance(node, FormatStr):
            return "char*"
        if isinstance(node, Call):
            fn_name = self.get_call_name(node)
            if fn_name in ("ln_format_int", "ln_format_float", "ln_str_concat"):
                return "char*"
        return "int64_t"

    def get_call_name(self, node: Call) -> str:
        if isinstance(node.func, Identifier):
            return node.func.name
        return ""

    def gen_expr(self, node: ASTNode) -> str:
        if isinstance(node, IntLit):
            return str(node.value)
        elif isinstance(node, FloatLit):
            return str(node.value)
        elif isinstance(node, StrLit):
            escaped = (
                node.value.replace("\\", "\\\\")
                .replace('"', '\\"')
                .replace("\n", "\\n")
                .replace("\t", "\\t")
            )
            return f'"{escaped}"'
        elif isinstance(node, BoolLit):
            return "1" if node.value else "0"
        elif isinstance(node, Identifier):
            return self.safe_name(node.name).replace("::", "__")
        elif isinstance(node, FormatStr):
            return self.gen_format_str(node)
        elif isinstance(node, BinOp):
            return self.gen_binop(node)
        elif isinstance(node, UnaryOp):
            expr = self.gen_expr(node.expr)
            if node.op == "!":
                return f"(!{expr})"
            elif node.op == "?":
                tmp = self.fresh_tmp()
                # If result looks like a pointer call, wrap it in Ok for the check
                self.emit(f"ln_result_t {tmp} = Ok((void*){expr});")
                self.emit(f"if (!{tmp}.ok) return {tmp};")
                return f"{tmp}.value"
            return f"({node.op}{expr})"
        elif isinstance(node, Assign):
            target = self.gen_expr(node.target)
            val = self.gen_expr(node.value)
            return f"({target} {node.op} {val})"
        elif isinstance(node, Call):
            return self.gen_call(node)
        elif isinstance(node, MethodCall):
            return self.gen_method_call(node)
        elif isinstance(node, FieldAccess):
            obj = self.gen_expr(node.obj)
            return f"{obj}->{node.field}"
        elif isinstance(node, Index):
            obj = self.gen_expr(node.obj)
            if isinstance(node.idx, RangeLit):
                start = self.gen_expr(node.idx.start) if node.idx.start else "0"
                return f"({obj} + {start})"
            idx = self.gen_expr(node.idx)
            return f"{obj}[{idx}]"
        elif isinstance(node, StructLit):
            return self.gen_struct_lit(node)
        elif isinstance(node, RangeLit):
            # Shouldn't be used as expr directly
            return "0"
        elif isinstance(node, Block):
            # Block expression - wrap in lambda-like pattern
            # Use comma operator for simple cases
            return "0 /* block expr */"
        else:
            return "0 /* unknown */"

    def gen_format_str(self, node: FormatStr) -> str:
        """Generate C code for format string"""
        template = node.template
        args = node.args

        if not args:
            escaped = (
                template.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
            )
            return f'"{escaped}"'

        # Simple substitution: replace {} with args
        # Build via snprintf
        c_fmt = template
        # Replace {} placeholders
        arg_strs = [self.gen_expr(a) for a in args]

        # Convert {} to appropriate formatter based on argument type
        # Build format string
        parts = template.split("{}")
        if len(parts) == len(args) + 1:
            # Build dynamic concat
            result = f'"{parts[0]}"'
            for i, arg in enumerate(args):
                arg_c = arg_strs[i]
                # Choose formatter based on AST node type
                ast_arg = args[i]
                if isinstance(ast_arg, FloatLit):
                    arg_str_c = f"ln_format_float({arg_c})"
                elif isinstance(ast_arg, StrLit) or isinstance(ast_arg, FormatStr):
                    arg_str_c = f"({arg_c})"  # already char*
                elif isinstance(ast_arg, BoolLit):
                    arg_str_c = f'(({arg_c}) ? "true" : "false")'
                elif isinstance(ast_arg, Identifier):
                    # Infer from known variable/function context: default safe stringify
                    # Use ln_format_int for generic identifiers; caller can cast if needed
                    arg_str_c = f"ln_format_int((int64_t){arg_c})"
                elif isinstance(ast_arg, Call):
                    fn_name = self.get_call_name(ast_arg)
                    if fn_name in ("ln_format_float", "ln_format_int", "ln_str_concat"):
                        arg_str_c = f"({arg_c})"  # already char*
                    else:
                        arg_str_c = f"ln_format_int((int64_t){arg_c})"
                else:
                    arg_str_c = f"ln_format_int((int64_t){arg_c})"
                # Escape the literal part for C string
                part_escaped = (
                    parts[i + 1]
                    .replace("\\", "\\\\")
                    .replace('"', '\\"')
                    .replace("\n", "\\n")
                    .replace("\r", "\\r")
                )
                result = f'ln_str_concat(ln_str_concat({result}, {arg_str_c}), "{part_escaped}")'
            return result
        else:
            # Fallback: just use template as-is
            escaped = (
                template.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\r", "\\r")
            )
            return f'"{escaped}"'

    def gen_binop(self, node: BinOp) -> str:
        left = self.gen_expr(node.left)
        right = self.gen_expr(node.right)
        op = node.op
        # String concatenation +
        if op == "+":
            # Check if either side looks like a string
            if (isinstance(node.left, StrLit) or isinstance(node.right, StrLit) or
                isinstance(node.left, FormatStr) or isinstance(node.right, FormatStr) or
                isinstance(node.left, BinOp) or isinstance(node.right, BinOp) or
                (isinstance(node.left, Identifier) and node.left.name in ("query_word", "translation", "cat", "path", "json_body", "html_body", "analysis", "morph_json", "input_word", "request_str", "response_buf", "body_ptr")) or
                (isinstance(node.right, Identifier) and node.right.name in ("query_word", "translation", "cat", "path", "json_body", "html_body", "analysis", "morph_json", "input_word", "request_str", "response_buf", "body_ptr"))):
                return f"ln_str_concat({left}, {right})"
        return f"({left} {op} {right})"

    def gen_call(self, node: Call) -> str:
        if isinstance(node.func, Identifier):
            name = node.func.name

            # Built-in print functions
            if name == "println":
                if node.args:
                    arg = self.gen_expr(node.args[0])
                    return f"println({arg})"
                return 'println("")'

            if name == "format":
                # Handle as format! if args available
                if node.args and isinstance(node.args[0], StrLit):
                    template = node.args[0].value
                    fmt_node = FormatStr(template, node.args[1:])
                    return self.gen_format_str(fmt_node)
                return '""'

            if name == "Ok":
                val = self.gen_expr(node.args[0]) if node.args else "0"
                return f"(ln_result_t){{1, (void*){val}, NULL}}"

            if name == "Err":
                val = self.gen_expr(node.args[0]) if node.args else '""'
                return f"(ln_result_t){{0, NULL, (char*){val}}}"

            if name == "print":
                if node.args:
                    arg = self.gen_expr(node.args[0])
                    return f"print_ln({arg})"
                return 'print_ln("")'

            if name in ("eprintln", "eprint"):
                if node.args:
                    arg = self.gen_expr(node.args[0])
                    return f"eprintln_fn({arg})"
                return 'eprintln_fn("")'

            # Standard functions
            args = [self.gen_expr(a) for a in node.args]
            args_str = ", ".join(args)

            # Map common Nova stdlib calls to C
            name_map = {
                "exit": "exit",
                "abort": "abort",
                "malloc": "malloc",
                "free": "free",
                "memcpy": "memcpy",
                "memset": "memset",
                "strlen": "strlen",
                "strcmp": "strcmp",
                "strstr": "strstr",
                "printf": "printf",
                "sprintf": "sprintf",
                "snprintf": "snprintf",
                "atoi": "atoi",
                "atoll": "atoll",
                "atof": "atof",
            }

            c_name = name_map.get(name, name)
            return f"{c_name}({args_str})"

        elif isinstance(node.func, FieldAccess):
            # Could be method or namespace::func
            obj = self.gen_expr(node.func.obj)
            method = node.func.field
            args = [self.gen_expr(a) for a in node.args]
            args_str = ", ".join(args)
            return f"{obj}__{method}({args_str})"

        else:
            func = self.gen_expr(node.func)
            args = [self.gen_expr(a) for a in node.args]
            args_str = ", ".join(args)
            return f"{func}({args_str})"

    def gen_method_call(self, node: MethodCall) -> str:
        obj = self.gen_expr(node.obj)
        args = [self.gen_expr(a) for a in node.args]
        args_str = ", ".join(args)

        # Common string/vec methods
        if node.method == "len":
            return f"ln_str_len({obj})"
        elif node.method == "push":
            return f"/* vec push {obj} {args_str} */ 0"
        elif node.method == "to_string":
            return f"(char*){obj}"
        elif node.method == "as_str":
            return obj
        elif node.method == "clone":
            return f"(strdup({obj}))"
        elif node.method == "unwrap":
            return f"({obj})"
        elif node.method == "is_ok":
            return f"({obj}).ok"
        elif node.method == "is_err":
            return f"(!({obj}).ok)"
        elif node.method == "into":
            return f"(char*)({obj})"
        elif node.method == "map_err":
            return f"({obj})"

        # Default: treat as struct method
        # Determine struct type - use obj directly
        all_args = ", ".join([obj] + ([args_str] if args_str else []))
        return f"/* method */{obj}__{node.method}({all_args})"

    def gen_struct_lit(self, node: StructLit) -> str:
        """Generate struct literal - heap allocated"""
        tmp = self.fresh_tmp()
        # Emit allocation
        self.emit(
            f"struct {node.name}* {tmp} = (struct {node.name}*)malloc(sizeof(struct {node.name}));"
        )
        for fname, fval in node.fields:
            val = self.gen_expr(fval)
            self.emit(f"{tmp}->{fname} = {val};")
        return tmp


# ===========================================================================
# COMPILER DRIVER
# ===========================================================================


class CompileError(Exception):
    pass


def compile_files(
    input_paths: List[str],
    output_path: str,
    verbose: bool = False,
    keep_c: bool = False,
    only_c: bool = False,
    extra_c_files: List[str] = None,
) -> bool:
    """Compile multiple .zn files into a single native binary"""
    extra_c_files = extra_c_files or []

    print(f"⚡ Nova Stage 0 Bootstrap Compiler")
    print(f"   Modules: {len(input_paths)} files")
    print(f"   Output:  {output_path}")
    print()

    combined_items = []

    for path in input_paths:
        try:
            if not os.path.exists(path):
                if verbose:
                    print(f"  [~] Skipping missing: {path}")
                continue
            with open(path, "r") as f:
                source = f.read()
            lexer = Lexer(source)
            tokens = lexer.tokenize()
            parser = Parser(tokens)
            program = parser.parse()
            combined_items.extend(program.items)
            if verbose:
                print(f"  [+] Parsed {os.path.basename(path)}")
        except Exception as e:
            print(f"❌ Error in {path}: {e}")
            return False

    program = Program(combined_items)

    # Code generation
    print("  [3/4] Generating Unified C code...")
    try:
        codegen = CodeGen()
        c_code = codegen.generate(program)
    except Exception as e:
        print(f"❌ Codegen error: {e}")
        import traceback

        traceback.print_exc()
        return False

    # Write C file
    c_path = output_path if only_c else output_path + "_tmp.c"

    with open(c_path, "w") as f:
        f.write(c_code)

    if only_c:
        print(f"  ✅ C code written to {c_path}")
        return True

    # Compile with GCC
    print("  [4/4] Compiling to native binary...")
    runtime_dir = os.path.dirname(os.path.abspath(__file__))
    io_c = os.path.join(runtime_dir, "nova_runtime_io.c")
    net_c = os.path.join(runtime_dir, "nova_runtime_net.c")

    # Fallback if not in same dir
    if not os.path.exists(io_c):
        runtime_dir = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "..", "runtime"
        )
        io_c = os.path.join(runtime_dir, "nova_runtime_io.c")
        net_c = os.path.join(runtime_dir, "nova_runtime_net.c")

    cmd = [
        "gcc",
        "-O2",
        "-std=c11",
        "-D_POSIX_C_SOURCE=200809L",
        "-D_DEFAULT_SOURCE",
        "-w",  # suppress warnings in generated code
    ]

    sources = [c_path, io_c]
    if os.path.exists(net_c):
        sources.append(net_c)

    # Automatically link AST runtime if it exists
    ast_c = os.path.join(runtime_dir, "nova_runtime_ast.c")
    if os.path.exists(ast_c):
        sources.append(ast_c)

    # Add extra C files
    for extra in extra_c_files:
        sources.append(extra)

    # Deduplicate sources by absolute path
    seen = set()
    deduped_sources = []
    for s in sources:
        abs_s = os.path.abspath(s)
        if abs_s not in seen:
            seen.add(abs_s)
            deduped_sources.append(s)

    cmd += deduped_sources
    
    cmd += [
        "-o",
        output_path,
        "-lm",
    ]

    if verbose:
        print(f"        {' '.join(cmd)}")

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"❌ GCC error:")
        print(result.stderr)
        return False

    if not keep_c:
        try:
            os.remove(c_path)
        except:
            pass

    print(f"\n  ✅ Compilation successful!")
    print(f"  🚀 Binary: {output_path}")
    print(f"     Run with: ./{output_path}")
    return True


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Nova Stage 0 Bootstrap Compiler")
    parser.add_argument("inputs", nargs="+", help="Input .zn files or directories")
    parser.add_argument("-o", "--output", help="Output binary path")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--emit-c", action="store_true", help="Only generate C code")
    parser.add_argument("--keep-c", action="store_true", help="Keep generated C file")

    args = parser.parse_args()

    # Collect all files
    all_files = []
    for item in args.inputs:
        if os.path.isdir(item):
            for root, _, files in os.walk(item):
                for f in files:
                    if f.endswith(".zn"):
                        all_files.append(os.path.join(root, f))
        elif os.path.isfile(item):
            all_files.append(item)

    zn_files = [f for f in all_files if f.endswith(".zn")]
    c_files = [f for f in all_files if f.endswith(".c")]

    if not zn_files:
        print("❌ No .zn input files found.")
        sys.exit(1)

    output = args.output or "a.out"
    success = compile_files(
        zn_files,
        output,
        verbose=args.verbose,
        keep_c=args.keep_c,
        only_c=args.emit_c,
        extra_c_files=c_files,
    )
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
