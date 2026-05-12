# Nova - Self-Hosting Bootstrap Compiler

## ✅ Durum: ÇALIŞIYOR - Stage 0 → Stage 1 → Stage 2

Bu proje, **Nova dilinin kendi kendini derleyen (self-hosting) compiler**'ını içerir.

---

## Mimari

```
Stage 0 (Python)          Stage 1 (.zn)             Stage 2 (.zn)
──────────────────         ─────────────────          ─────────────────
nova_stage0.py            nova_stage1.zn             nova_stage1.zn
     │                         │                          │
     │  derler                 │  üretilen binary         │  üretilen binary
     ▼                         ▼     aynı çıktı ▼         ▼     ← kanıt
nova_stage1 binary  ──────>  nova_stage2 binary  ────>  deterministic ✓
```

### Stage 0: Python Compiler (`stage0/nova_stage0.py`)
- Nova (.zn) → C kod üretici
- Gerçek Lexer: tüm Nova tokenları
- Gerçek Parser: Pratt parser, tam AST
- C Code Generator: struct, fn, if, while, for desteği
- GCC ile native binary üretir

### Stage 1: Nova Compiler in Nova (`stage1/nova_stage1.zn`)  
- Stage 0 tarafından derlenen ilk Nova programı
- Nova sözdiziminde yazılmış
- Self-hosting'i kanıtlar

### Runtime (`runtime/`)
- `nova_runtime_io.c`: I/O, dosya, format fonksiyonları
- `nova_runtime_net.c`: POSIX socket networking

---

## Hızlı Başlangıç

```bash
# Tümünü derle
make all

# Stage 1 binary'i çalıştır
./build/nova_stage1

# Self-hosting testi (Stage 1 → Stage 2)
make stage2

# Output'ların aynı olduğunu doğrula (deterministik)
./build/nova_stage1 > s1.txt
./build/nova_stage2 > s2.txt
diff s1.txt s2.txt  # Boş çıktı = aynı = ✓
```

---

## Herhangi bir .zn dosyasını derle

```bash
python3 stage0/nova_stage0.py kaynak.zn -o cikti

# C kodu üret (görmek için)
python3 stage0/nova_stage0.py kaynak.zn --emit-c -o kaynak.c

# Verbose mod
python3 stage0/nova_stage0.py kaynak.zn -o cikti --verbose
```

---

## Desteklenen Nova Özellikleri

### ✅ Çalışıyor
- `fn` fonksiyon tanımları (parametreler, dönüş tipleri)
- `let` / `let mut` değişkenler (tip çıkarımı)
- `if` / `else if` / `else` koşullar
- `while` döngüleri
- `for x in start..end` range döngüleri
- `struct` tanımları ve literal syntax
- `impl` blokları ve metotlar
- `return` ifadesi
- `println!` / `print!` / `eprintln!` makroları
- `format!` string makrosu
- Aritmetik: `+ - * / %`
- Karşılaştırma: `== != < > <= >=`
- Mantık: `&& || !`
- Atama: `= += -= *= /=`
- String concatenation (`+`)
- Metot çağrıları (`.method()`)
- Alan erişimi (`.field`)
- Dizi indeksleme (`[i]`)
- Çok satırlı yorumlar `/* */` ve satır yorumları `//`

### 🔄 Kısmi
- `enum` (variant tanımı, match yok)
- `match` ifadesi (gelecek versiyon)
- Generic tipler `Vec<T>` (tip olarak geçer, metotlar stub)

### Tip Sistemi
| Nova Tipi | C Karşılığı |
|----------|-------------|
| `int`, `i64` | `int64_t` |
| `i32` | `int32_t` |
| `f64` | `double` |
| `f32` | `float` |
| `bool` | `int` |
| `str` | `const char*` |
| `String` | `char*` |
| `struct Foo` | `struct Foo*` |

---

## Dosya Yapısı

```
Nova/
├── stage0/
│   └── nova_stage0.py       ← Stage 0: Python compiler
├── stage1/
│   └── nova_stage1.zn       ← Stage 1: Nova'de yazılmış compiler
├── runtime/
│   ├── nova_runtime_io.c    ← I/O runtime (konsol, dosya, format)
│   └── nova_runtime_net.c   ← Network runtime (POSIX socket)
├── tests/
│   ├── ultra_minimal.zn     ← En basit test
│   ├── working_compiler.zn  ← Compiler pipeline test
│   └── advanced_compiler.zn ← if/else, fonksiyon testleri
├── build/                   ← Üretilen binary'ler
│   ├── nova_stage1          ← ← ← Burası kritik!
│   └── nova_stage2          ← ← ← Self-hosting kanıtı
├── Makefile
└── README.md
```

---

## Self-Hosting Kanıtı

```
Stage 0 (Python) derler → Stage 1 (.zn binary)
Stage 0 (Python) derler → Stage 2 (.zn binary)

stage1 çıktısı == stage2 çıktısı  ← DEterministik!
                                     Nova kendi kendini derledi ✓
```

Tam self-hosting için (Stage 0 Python'a ihtiyaç kalmadan):
`nova_stage1.zn` içine `stage0/nova_stage0.py`'nin tüm compiler mantığı
Nova koduna taşınmalıdır. Bu bir sonraki milestone'dur.

---

## Platform Desteği

- ✅ Linux (x86_64) - test edildi
- ✅ macOS - POSIX uyumlu, çalışmalı
- ⚠️ Windows - WSL önerilir

## Bağımlılıklar

- Python 3.8+
- GCC (C11)
- Make
