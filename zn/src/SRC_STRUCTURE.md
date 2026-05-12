# Nova `zn/src` klasör yapısı

Bu belge `zn/src` altındaki modül hiyerarşisini ve frontend birleştirmesini açıklar.

## Genel yapı

```
zn/src/
├── compiler/           # Derleyici (frontend + backend + optimizer + …)
│   ├── frontend/       # Tek frontend (v10 AST/IR + dil frontend’i)
│   ├── compiler/       # v10 compiler lib (optimization, codegen, runtime, utils)
│   ├── backend/        # LLVM, CUDA, WASM, …
│   ├── optimizer/     # const_eval, auto_parallel, …
│   ├── bootstrap/     # Stage0 giriş noktaları
│   └── …
├── stdlib/             # Standart kütüphane
├── runtime/            # Runtime
├── tools/              # Araçlar (infra, formal, …)
└── cli/                # CLI
```

## Frontend merge (Mart 2025)

**Önce:** İki ayrı frontend vardı:

- `compiler/frontend/` — Dil frontend’i: lexer, parser impl, ast, type_checker, semantic_analyzer, core, web, mobile, desktop, advanced.
- `compiler/compiler/frontend/` — v10 AST/IR: parser (Token, Program), errors (Diagnostic), span, hir, ir, mir.

**Sonra:** Tek `compiler/frontend/` altında birleştirildi:

| Konum | İçerik |
|--------|--------|
| `frontend/parser.zn` | v10 Token, Program, AST (eski compiler/compiler/frontend) |
| `frontend/errors.zn` | v10 Diagnostic, DiagnosticBuilder (eski compiler/compiler/frontend) |
| `frontend/span.zn` | v10 Span, Spanned (eski compiler/compiler/frontend) |
| `frontend/ir.zn`, `hir.zn`, `mir.zn` | v10 IR (eski compiler/compiler/frontend) |
| `frontend/core/` | Lexer, Parser impl, ast, type_checker, semantic_analyzer, errors (LexError, ParseError, …), ir_generator, … |
| `frontend/web`, `mobile`, `desktop`, `advanced` | Platform / gelişmiş frontend |

- **Referanslar:** `crate::frontend::parser` / `crate::frontend::core::parser` ayrımı korunur (v10 AST vs dil parser impl). `compiler::compiler` artık `open use crate::frontend` ile aynı frontend’i kullanır.
- **Eski konum:** `compiler/compiler/frontend/` kaldırıldı; dosyalar `compiler/frontend/` köküne taşındı.

## Önemli modül yolları

- `crate::frontend::core` — Lexer, Parser, TypeChecker, SemanticAnalyzer, ast, errors (CompilerError), …
- `crate::frontend::parser` — v10 Token, Program (AST)
- `crate::frontend::errors` — v10 Diagnostic
- `crate::frontend::span` — v10 Span
- `compiler::frontend::ast` — AST tipleri (core içinde)
- `compiler::frontend::core` — Dil frontend core
