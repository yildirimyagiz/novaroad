# Nova Compiler main.c - Özellik Analizi

## 📊 Mevcut Özellikler

### ✅ Çalışan Modlar:
1. **MODE_RUN** (Line 245-401) - ✅ TAM İMPLEMENTE
   - Lex → Parse → Semantic → Codegen → VM Execute
   - Bytecode üretir ve çalıştırır
   - `--jit` flag ile JIT execution
   - Return code: 0 (başarılı)

2. **MODE_HELP** (Line 208-210) - ✅ TAM İMPLEMENTE
   - `--help` çalışıyor
   - Tüm komutları gösteriyor

3. **MODE_VERSION** (Line 212-215) - ✅ TAM İMPLEMENTE
   - `--version` çalışıyor
   - LLVM, Borrow Checker bilgileri gösteriyor

4. **MODE_PARSE** (Line 404-432) - ✅ KISMEN İMPLEMENTE
   - Default mode (sadece file verince)
   - `nova_compile_source()` çağırıyor
   - Parse + type check yapıyor ama output yok

### ❌ EKSİK Modlar:

1. **MODE_BUILD** - ❌ İMPLEMENTASYON YOK!
   - Line 32'de tanımlı
   - Line 113'te parse ediliyor
   - AMA kod içinde kullanılmıyor!
   - Çalıştırılabilir executable üretmiyor

2. **MODE_CHECK** - ❌ İMPLEMENTASYON YOK!
   - Line 119'da parse ediliyor
   - Sadece type check yapmalı
   - AMA ayrı implementation yok (default mode ile aynı)

### 🔧 Emit Özellikleri:

1. **EMIT_AST** - ❌ İMPLEMENTASYON YOK
   - Flag parse ediliyor ama kullanılmıyor

2. **EMIT_IR** - ❌ İMPLEMENTASYON YOK
   - Flag parse ediliyor ama LLVM IR yazdırmıyor

3. **EMIT_OBJ** - ❌ İMPLEMENTASYON YOK
   - Object file üretmiyor

4. **EMIT_ASM** - ❌ İMPLEMENTASYON YOK
   - Assembly üretmiyor

## 🎯 SORUN TANIMI

**Compiler çalışıyor ama sadece bytecode execute ediyor!**

### Mevcut Durum:
```
nova run test.zn       ✅ Çalışıyor (bytecode + VM)
nova --jit test.zn     ✅ Çalışıyor (bytecode + VM)
nova check test.zn     ⚠️ Parse yapıyor ama output yok
nova build test.zn     ❌ ÇALIŞMIYOR (implementasyon yok!)
nova test.zn -o app    ❌ ÇALIŞMIYOR (implementasyon yok!)
--emit-ir              ❌ ÇALIŞMIYOR (implementasyon yok!)
--emit-obj             ❌ ÇALIŞMIYOR (implementasyon yok!)
```

## 🔍 EKSİK KOD

### 1. MODE_BUILD için gerekli kod:
```c
// Line 245'ten sonra eklenmelİ:
if (opts.mode == MODE_BUILD) {
    // LLVM backend kullan
    // Object file üret
    // Link et
    // Executable oluştur
}
```

### 2. EMIT flags için gerekli kod:
```c
// Her mode'da emit kontrolü:
if (opts.emit == EMIT_IR) {
    codegen_dump_ir(codegen);
}
if (opts.emit == EMIT_OBJ) {
    codegen_emit_object(codegen, output_file);
}
```

## 📋 JIRA İÇİN TODO LİSTESİ

### Epic: NOVA-COMPILER-1 "Complete Build System Implementation"

#### Story 1: NOVA-COMPILER-2 "Implement MODE_BUILD"
- [ ] LLVM backend entegrasyonu
- [ ] Object file generation
- [ ] Linking to executable
- [ ] Output file (-o) handling

#### Story 2: NOVA-COMPILER-3 "Implement EMIT flags"
- [ ] --emit-ast (AST dump)
- [ ] --emit-ir (LLVM IR output)
- [ ] --emit-obj (Object file)
- [ ] --emit-asm (Assembly output)

#### Story 3: NOVA-COMPILER-4 "Fix MODE_CHECK"
- [ ] Type check only mode
- [ ] No bytecode generation
- [ ] Exit with errors only

#### Bug 1: NOVA-COMPILER-5 "MODE_RUN doesn't show output"
- [ ] VM execute edilen kod sonucunu göstermiyor
- [ ] Return value yazdırılmalı
- [ ] Printf/println desteği eklenmeli

## 💡 ÖNCELİKLER

### P0 - Critical (Hemen yapılmalı):
1. MODE_BUILD implementasyonu - **executable üretme**
2. EMIT_OBJ implementasyonu - **object file üretme**

### P1 - High (Bu hafta):
3. EMIT_IR implementasyonu - **LLVM IR gösterme**
4. VM output handling - **sonuçları gösterme**

### P2 - Medium (Bu ay):
5. EMIT_AST implementasyonu
6. EMIT_ASM implementasyonu
7. MODE_CHECK fix

## 🚀 SONUÇ

**Compiler %60 tamamlanmış:**
- ✅ Lexer, Parser, Semantic, Codegen, VM: Çalışıyor
- ❌ Native executable generation: Eksik
- ❌ LLVM backend usage: Kullanılmıyor
- ❌ Output formatting: Eksik
