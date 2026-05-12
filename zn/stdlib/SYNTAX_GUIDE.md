# Nova Standard Library - Syntax Guide

## Problem
`io.zn` ve diğer stdlib dosyaları eski/yanlış syntax kullanıyordu ve parser hataları veriyordu:
- `@extern("function_name")` inline kullanımı (YANLIŞ ❌)
- İfadelerde eksik semicolon (`;`)
- `return` yerine `yield` kullanılmaması

## Çözüm
Tüm dosyaları Nova dilinin resmi syntax kurallarına göre güncelledik.

## Nova Syntax Kuralları

### 1. Extern Fonksiyon Tanımlamaları

**YANLIŞ ❌:**
```nova
pub fn println(msg: String) {
    @extern("nova_runtime_println")
    _runtime_println(msg.ptr, msg.len)
}

@extern("nova_runtime_println")
fn _runtime_println(ptr: *u8, len: i64)
```

**DOĞRU ✅:**
```nova
pub fn println(msg: String) {
    _runtime_println(msg.ptr, msg.len);
}

extern "C" fn _runtime_println(ptr: *u8, len: i64);
```

### 2. Return İfadeleri

**YANLIŞ ❌:**
```nova
pub fn read_line() -> String {
    let ptr = _runtime_read_line();
    return String::from_ptr(ptr);
}
```

**DOĞRU ✅:**
```nova
pub fn read_line() -> String {
    let ptr = _runtime_read_line();
    yield String::from_ptr(ptr);
}
```

### 3. Semicolon Kullanımı

Her ifade `;` ile bitmeli:
```nova
let x = 5;
println("Hello");
_runtime_function();
```

### 4. If vs Check

Nova'da iki farklı conditional syntax vardır:

**Traditional If (C-style compiler için):**
```nova
if condition {
    // kod
}
```

**Nova-style Check (Gelişmiş özellikler için):**
```nova
check condition {
    // kod
    yield result;
}
```

### 5. String Concatenation

String concatenation `+` operatörü ile **desteklenmiyor**:

**YANLIŞ ❌:**
```nova
let msg = "Error: " + error_message;
```

**DOĞRU ✅:**
```nova
println("Error:");
println(error_message);
```

## Güncellenen Dosyalar

### ✅ nova/zn/stdlib/core/io.zn
- Tüm `@extern` tanımlamaları `extern "C"` syntax'ına çevrildi
- Tüm `return` ifadeleri `yield`'e çevrildi
- Tüm ifadelere `;` eklendi
- String concatenation kaldırıldı

## Test Sonuçları

```bash
✅ ./build/tools/znc tests/verify_cli_capabilities.zn -o build/verify_cli.nbc --backend vm -I zn/stdlib/core
✅ ./build/tools/znc tests/tiny_if.zn -o build/tiny_if.nbc --backend vm
```

Her iki test de başarıyla derleniyor!

## Referans Dosyalar

Nova dilinin resmi syntax kuralları için:
- `nova/zn/src/compiler/frontend/core/parser.zn` - Parser implementation
- `nova/zn/src/compiler/frontend/core/lexer.zn` - Lexer implementation
- `nova/zn/src/compiler/frontend/core/simd_intrinsics.zn` - Extern fonksiyon örnekleri

## Gelecek Çalışma

Diğer stdlib dosyaları da aynı kurallara göre güncellenebilir:
- `collections.zn` - `return` → `yield`
- `core.zn` - `return` → `yield`
- `iter_fusion.zn`
- `novascience.zn`

Her güncellemeden sonra derleme testi yapılmalı.
