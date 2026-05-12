#!/usr/bin/env python3
"""
Novae 0 Bootstrap Compiler
=========================================
.zn kaynak kodu → C kodu → Native binary

Bu compiler:
  1. Novalexer
  2. Nova (AST)
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
            self.peek().isdigit() or self.peek() == "."
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

            # Number
            if ch.isdigit():
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
    name: str
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

    def parse(self) -> Program:
        items = []
        while not self.check(TT.EOF):
            item = self.parse_top_level()
            if item:
                items.append(item)
        return Program(items)

    def parse_top_level(self) -> Optional[ASTNode]:
        # Skip 'use' declarations
        if self.check(TT.KW_USE):
            self.advance_to_semicolon()
            return None

        is_pub = bool(self.match(TT.KW_PUB))

        if self.check(TT.KW_FN):
            return self.parse_fn(is_pub)
        elif self.check(TT.KW_STRUCT):
            return self.parse_struct()
        elif self.check(TT.KW_IMPL):
            return self.parse_impl()
        elif self.check(TT.KW_ENUM):
            return self.parse_enum()
        elif self.check(TT.KW_EXTERN):
            self.advance_to_semicolon()
            return None
        else:
            # Skip unknown
            self.pos += 1
            return None

    def advance_to_semicolon(self):
        depth = 0
        while not self.check(TT.EOF):
            if self.check(TT.LBRACE):
                depth += 1
            elif self.check(TT.RBRACE):
                if depth == 0:
                    break
                depth -= 1
            elif self.check(TT.SEMICOLON) and depth == 0:
                self.pos += 1
                break
            self.pos += 1

    def parse_fn(self, is_pub=False) -> FnDecl:
        self.expect(TT.KW_FN)
        name = self.expect(TT.IDENT).value
        self.expect(TT.LPAREN)
        params = []
        self_param = False

        while not self.check(TT.RPAREN) and not self.check(TT.EOF):
            # Handle 'self', 'mut self', '&self', '&mut self'
            if self.check(TT.KW_SELF):
                self_param = True
                self.pos += 1
            elif self.check(TT.AMPERSAND):
                self.pos += 1
                if self.check(TT.KW_MUT):
                    self.pos += 1
                if self.check(TT.KW_SELF):
                    self_param = True
                    self.pos += 1
                else:
                    # &Type param
                    pname = self.expect(TT.IDENT).value
                    self.expect(TT.COLON)
                    ptype = self.parse_type()
                    params.append((pname, ptype))
            elif self.check(TT.KW_MUT):
                self.pos += 1
                if self.check(TT.KW_SELF):
                    self_param = True
                    self.pos += 1
                else:
                    pname = self.expect(TT.IDENT).value
                    self.expect(TT.COLON)
                    ptype = self.parse_type()
                    params.append((pname, ptype))
            else:
                pname = self.expect(TT.IDENT).value
                self.expect(TT.COLON)
                ptype = self.parse_type()
                params.append((pname, ptype))

            if not self.match(TT.COMMA):
                break

        self.expect(TT.RPAREN)

        ret_type = None
        if self.match(TT.ARROW):
            ret_type = self.parse_type()

        body = self.parse_block()
        return FnDecl(name, params, ret_type, body, is_pub, self_param)

    def parse_type(self) -> str:
        """Parse type annotation, return as string"""
        parts = []
        if self.check(TT.AMPERSAND):
            parts.append("&")
            self.pos += 1
            if self.check(TT.KW_MUT):
                parts.append("mut ")
                self.pos += 1

        if self.check(TT.IDENT):
            name = self.pos
            type_name = self.tokens[self.pos].value
            self.pos += 1
            parts.append(type_name)
            # Generic params
            if self.check(TT.LT):
                self.pos += 1
                inner = self.parse_type()
                parts.append(f"<{inner}>")
                self.expect(TT.GT)
            # Path ::
            while self.check(TT.COLON_COLON):
                self.pos += 1
                seg = self.expect(TT.IDENT).value
                parts.append(f"::{seg}")
        elif self.check(TT.LPAREN):
            # Tuple type - skip
            depth = 1
            parts.append("()")
            self.pos += 1
            while depth > 0 and not self.check(TT.EOF):
                if self.check(TT.LPAREN):
                    depth += 1
                elif self.check(TT.RPAREN):
                    depth -= 1
                self.pos += 1
        else:
            parts.append("i64")  # fallback

        return "".join(parts)

    def parse_struct(self) -> StructDecl:
        self.expect(TT.KW_STRUCT)
        # Handle "Nova(two tokens)
        name_parts = [self.expect(TT.IDENT).value]
        # Check for space-separated name (e.g., "Nova
        while (
            self.check(TT.IDENT)
            and self.tokens[self.pos - 1].col + len(name_parts[-1])
            == self.tokens[self.pos].col - 1
        ):
            name_parts.append(self.tokens[self.pos].value)
            self.pos += 1
        name = "".join(name_parts)
        # Actually just grab the single ident
        name = name_parts[0]

        self.expect(TT.LBRACE)
        fields = []
        while not self.check(TT.RBRACE) and not self.check(TT.EOF):
            fname = self.expect(TT.IDENT).value
            self.expect(TT.COLON)
            ftype = self.parse_type()
            fields.append((fname, ftype))
            self.match(TT.COMMA)
        self.expect(TT.RBRACE)
        return StructDecl(name, fields)

    def parse_impl(self) -> ImplBlock:
        self.expect(TT.KW_IMPL)
        # Consume type name (may be multi-word)
        name = self.expect(TT.IDENT).value
        # Skip extra idents (e.g. "NovaCompiler")
        while self.check(TT.IDENT):
            name += self.tokens[self.pos].value
            self.pos += 1

        self.expect(TT.LBRACE)
        methods = []
        while not self.check(TT.RBRACE) and not self.check(TT.EOF):
            is_pub = bool(self.match(TT.KW_PUB))
            if self.check(TT.KW_FN):
                methods.append(self.parse_fn(is_pub))
            else:
                self.pos += 1  # skip unknown
        self.expect(TT.RBRACE)
        return ImplBlock(name, methods)

    def parse_enum(self) -> EnumDecl:
        self.expect(TT.KW_ENUM)
        name = self.expect(TT.IDENT).value
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
        self.expect(TT.RBRACE)
        return EnumDecl(name, variants)

    def parse_block(self) -> List[ASTNode]:
        self.expect(TT.LBRACE)
        stmts = []
        while not self.check(TT.RBRACE) and not self.check(TT.EOF):
            stmt = self.parse_statement()
            if stmt:
                stmts.append(stmt)
        self.expect(TT.RBRACE)
        return stmts

    def parse_statement(self) -> Optional[ASTNode]:
        if self.check(TT.KW_LET):
            return self.parse_let()
        elif self.check(TT.KW_RETURN):
            return self.parse_return()
        elif self.check(TT.KW_IF):
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
        elif self.check(TT.RBRACE):
            return None
        else:
            expr = self.parse_expr()
            self.match(TT.SEMICOLON)
            return ExprStmt(expr)

    def parse_let(self) -> LetStmt:
        self.expect(TT.KW_LET)
        is_mut = bool(self.match(TT.KW_MUT))
        name = self.expect(TT.IDENT).value
        type_ann = None
        if self.match(TT.COLON):
            type_ann = self.parse_type()
        value = None
        if self.match(TT.EQ):
            value = self.parse_expr()
        self.match(TT.SEMICOLON)
        return LetStmt(name, type_ann, value, is_mut)

    def parse_return(self) -> ReturnStmt:
        self.expect(TT.KW_RETURN)
        if self.check(TT.SEMICOLON) or self.check(TT.RBRACE):
            self.match(TT.SEMICOLON)
            return ReturnStmt(None)
        val = self.parse_expr()
        self.match(TT.SEMICOLON)
        return ReturnStmt(val)

    def parse_if(self) -> IfStmt:
        self.expect(TT.KW_IF)
        cond = self.parse_expr()
        then_b = self.parse_block()
        else_b = None
        if self.match(TT.KW_ELSE):
            if self.check(TT.KW_IF):
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
        self.expect(TT.KW_FOR)
        var = self.expect(TT.IDENT).value
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
        if self.check(TT.BANG):
            self.pos += 1
            return UnaryOp("!", self.parse_unary())
        if self.check(TT.MINUS):
            self.pos += 1
            return UnaryOp("-", self.parse_unary())
        return self.parse_postfix()

    def parse_postfix(self) -> ASTNode:
        expr = self.parse_primary()
        while True:
            if self.check(TT.DOT):
                self.pos += 1
                field = self.expect(TT.IDENT).value
                if self.check(TT.LPAREN):
                    args = self.parse_args()
                    expr = MethodCall(expr, field, args)
                else:
                    expr = FieldAccess(expr, field)
            elif self.check(TT.LBRACKET):
                self.pos += 1
                idx = self.parse_expr()
                self.expect(TT.RBRACKET)
                expr = Index(expr, idx)
            elif self.check(TT.LPAREN):
                args = self.parse_args()
                expr = Call(expr, args)
            elif self.check(TT.COLON_COLON):
                self.pos += 1
                name = self.expect(TT.IDENT).value
                if self.check(TT.LPAREN):
                    args = self.parse_args()
                    expr = Call(FieldAccess(expr, name), args)
                else:
                    expr = FieldAccess(expr, name)
            else:
                break
        return expr

    def parse_args(self) -> List[ASTNode]:
        self.expect(TT.LPAREN)
        args = []
        while not self.check(TT.RPAREN) and not self.check(TT.EOF):
            args.append(self.parse_expr())
            if not self.match(TT.COMMA):
                break
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

        else:
            # Skip and return a null literal
            self.pos += 1
            return IntLit(0)

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
            fname = self.expect(TT.IDENT).value
            self.expect(TT.COLON)
            fval = self.parse_expr()
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
    "void": "void",
    "char": "char",
}


class CodeGen:
    def __init__(self):
        self.output: List[str] = []
        self.indent_level = 0
        self.structs: Dict[str, StructDecl] = {}
        self.functions: Dict[str, FnDecl] = {}
        self.temp_count = 0
        self.string_pool: Dict[str, str] = {}  # value -> var name
        self.string_decls: List[str] = []

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
            elif isinstance(item, FnDecl):
                self.functions[item.name] = item
            elif isinstance(item, ImplBlock):
                for m in item.methods:
                    self.functions[f"{item.type_name}__{m.name}"] = m

        header = self.gen_header()
        body_parts = []

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
/* Generated by Nova0 Compiler */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* ---- Novae Types ---- */
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
"""

    def gen_struct(self, s: StructDecl) -> str:
        lines = [f"struct {s.name} {{"]
        for fname, ftype in s.fields:
            ctype = self.type_to_c(ftype)
            lines.append(f"    {ctype} {fname};")
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
            params.append(f"{self.type_to_c(ptype)} {pname}")
        return f'{ret} {fn.name}({", ".join(params) if params else "void"})'

    def gen_method_sig(self, struct_name: str, fn: FnDecl) -> str:
        ret = self.type_to_c(fn.ret_type) if fn.ret_type else "void"
        params = []
        if fn.self_param:
            params.append(f"struct {struct_name}* _self")
        for pname, ptype in fn.params:
            params.append(f"{self.type_to_c(ptype)} {pname}")
        return (
            f'{ret} {struct_name}__{fn.name}({", ".join(params) if params else "void"})'
        )

    def gen_fn_forward(self, fn: FnDecl) -> str:
        return self.gen_fn_sig(fn) + ";"

    def gen_method_forward(self, struct_name: str, fn: FnDecl) -> str:
        return self.gen_method_sig(struct_name, fn) + ";"

    def gen_fn(self, fn: FnDecl) -> str:
        lines = [self.gen_fn_sig(fn) + " {"]
        self.indent_level = 1
        self.output = []
        for stmt in fn.body:
            self.gen_stmt(stmt)
        # Add return 0 to main if not already there
        if fn.name == "main":
            self.output.append("    return 0;")
        for line in self.output:
            lines.append(line)
        lines.append("}")
        self.output = []
        self.indent_level = 0
        return "\n".join(lines)

    def gen_method(self, struct_name: str, fn: FnDecl) -> str:
        lines = [self.gen_method_sig(struct_name, fn) + " {"]
        self.indent_level = 1
        self.output = []
        for stmt in fn.body:
            self.gen_stmt(stmt)
        for line in self.output:
            lines.append(line)
        lines.append("}")
        self.output = []
        self.indent_level = 0
        return "\n".join(lines)

    def gen_stmt(self, node: ASTNode):
        if isinstance(node, LetStmt):
            ctype = self.type_to_c(node.type_ann) if node.type_ann else "int64_t"
            if node.value:
                val = self.gen_expr(node.value)
                # Auto-detect type from value if unknown
                if not node.type_ann:
                    ctype = self.infer_c_type(node.value)
                self.emit(f"{ctype} {node.name} = {val};")
            else:
                self.emit(f"{ctype} {node.name} = 0;")

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
                    f"for (int64_t {node.var} = {start}; {node.var} < {end}; {node.var}++) {{"
                )
            else:
                # Generic iteration (treat as array)
                iter_e = self.gen_expr(node.iter)
                self.emit(f"/* for {node.var} in {iter_e} - simplified */")
                self.emit(f"for (int64_t _i = 0; _i < 0; _i++) {{")
                self.emit(f"    int64_t {node.var} = 0; /* TODO: array iteration */")
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
            return node.name
        elif isinstance(node, FormatStr):
            return self.gen_format_str(node)
        elif isinstance(node, BinOp):
            return self.gen_binop(node)
        elif isinstance(node, UnaryOp):
            expr = self.gen_expr(node.expr)
            if node.op == "!":
                return f"(!{expr})"
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

        # Convert {} to %s/%lld/%g based on... just use %s and stringify everything
        # Build format string
        parts = template.split("{}")
        if len(parts) == len(args) + 1:
            # Build dynamic concat
            result = f'"{parts[0]}"'
            for i, arg in enumerate(args):
                arg_c = arg_strs[i]
                # Try to format the arg
                arg_str_c = f"ln_format_int({arg_c})"  # default
                result = f'ln_str_concat(ln_str_concat({result}, {arg_str_c}), "{parts[i+1]}")'
            return result
        else:
            # Fallback: just use first arg as string
            return f'"{template}"'

    def gen_binop(self, node: BinOp) -> str:
        left = self.gen_expr(node.left)
        right = self.gen_expr(node.right)
        op = node.op
        # String concatenation +
        if op == "+":
            # Check if either side looks like a string
            if isinstance(node.left, StrLit) or isinstance(node.right, StrLit):
                return f"ln_str_concat({left}, {right})"
            if isinstance(node.left, FormatStr) or isinstance(node.right, FormatStr):
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
                "strlen": "strlen",
                "strcmp": "strcmp",
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


def compile_file(
    source_path: str,
    output_path: str,
    verbose: bool = False,
    keep_c: bool = False,
    only_c: bool = False,
) -> bool:
    """Compile a .zn file to a native binary"""

    print(f"⚡ Nova0 Compiler")
    print(f"   Input:  {source_path}")
    print(f"   Output: {output_path}")
    print()

    # Read source
    try:
        with open(source_path, "r") as f:
            source = f.read()
    except Exception as e:
        print(f"❌ Cannot read file: {e}")
        return False

    # Lex
    print("  [1/4] Lexing...")
    try:
        lexer = Lexer(source)
        tokens = lexer.tokenize()
        if verbose:
            print(f"        {len(tokens)} tokens")
    except LexError as e:
        print(f"❌ Lex error: {e}")
        return False

    # Parse
    print("  [2/4] Parsing...")
    try:
        parser = Parser(tokens)
        program = parser.parse()
        fns = sum(1 for i in program.items if isinstance(i, FnDecl))
        structs = sum(1 for i in program.items if isinstance(i, StructDecl))
        impls = sum(1 for i in program.items if isinstance(i, ImplBlock))
        if verbose:
            print(f"        {fns} functions, {structs} structs, {impls} impl blocks")
    except ParseError as e:
        print(f"❌ Parse error: {e}")
        return False

    # Code generation
    print("  [3/4] Generating C code...")
    try:
        codegen = CodeGen()
        c_code = codegen.generate(program)
    except Exception as e:
        print(f"❌ Codegen error: {e}")
        import traceback

        traceback.print_exc()
        return False

    # Write C file
    if only_c:
        c_path = output_path
    else:
        # Always write C to a temp dir or alongside output, not alongside source
        c_path = output_path + "_tmp.c"

    with open(c_path, "w") as f:
        f.write(c_code)

    if only_c:
        print(f"  ✅ C code written to {c_path}")
        return True

    # Compile with GCC
    print("  [4/4] Compiling to native binary...")
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
        c_path,
        io_c,
        net_c,
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

    parser = argparse.ArgumentParser(
        description="Nova0 Bootstrap Compiler",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s hello.zn -o hello
  %(prog)s compiler.zn -o nova_stage1
  %(prog)s source.zn --emit-c          # Only generate C code
  %(prog)s source.zn -o out --keep-c   # Keep generated C file
        """,
    )
    parser.add_argument("input", help="Input .zn file")
    parser.add_argument("-o", "--output", help="Output binary path")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--emit-c", action="store_true", help="Only generate C code")
    parser.add_argument("--keep-c", action="store_true", help="Keep generated C file")

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"❌ File not found: {args.input}")
        sys.exit(1)

    output = args.output
    if not output:
        if args.emit_c:
            output = args.input.replace(".zn", ".c")
        else:
            output = args.input.replace(".zn", "")
            if output == args.input:
                output = args.input + ".out"

    success = compile_file(
        args.input, output, verbose=args.verbose, keep_c=args.keep_c, only_c=args.emit_c
    )

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
