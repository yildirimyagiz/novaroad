# Nova Self-Hosting Yol Haritası

## Mevcut durum (Stage 0 → Stage 1)

### Tamamlananlar
- **Stage 0 (Python)**: Lexer, parser, C codegen, GCC ile native binary. `zn/` → C → `a.out`.
- **Parse genişletmeleri**: `each (a,b) in`, `&var self`, turbofish `::<T>()`, path struct literal, `check let`, `fn()/Fn() -> Ret`, `impl Trait`, `ident!()`, range `a..b`, unary `*`, unit `()`, forward decl `fn foo();`, enum/struct generics ve `derives`, extern block skip, EOF toleransı, ASCII-only sayılar.
- **native_linker.zn**: Hem `zn/src/compiler/frontend/core/` hem `zn/src/tools/infra/` için compiler frontend sözdizimine uygun stub yazıldı.
- **tools/infra**: `integration_plan`, `incremental`, `debugger`, `config`, `build_system`, `bootstrap` vb. minimal stub’lar eklendi.

### Şu an takılan nokta
- **lexer.zn ~136**: `Expected RBRACE, got DOT` — muhtemelen `match self.cur() { Some(ch) => { check ... } None => {} }` gibi iç içe `match` + `check` blokları stage0 parser’da tam modellenmediği için; bir sonraki adımda `match` arm’larının `pat => block` olarak özel parse edilmesi gerekebilir.

---

## Self-hosted yapıya uzaklık (kabaca)

| Katman | Durum | Not |
|--------|--------|-----|
| **1. Stage 0 (mevcut)** | ✅ | Python: .zn → C → binary. Sınırlı syntax. |
| **2. Tüm compiler .zn’lerin parse’ı** | 🟡 | ~255 dosya; çoğu geçiyor, birkaç edge case (lexer.zn match/check) kaldı. |
| **3. Codegen (C/LLVM)** | 🟡 | Basit C üretiliyor; Vec, Result, trait, match tam değil. |
| **4. Stage 1 binary** | 🟡 | Parse + codegen tam bitince: “Nova ile yazılmış” tek bir compiler binary’si. |
| **5. Stage 1 = kendini derleyebilir** | 🔴 | Stage 1’in kendisi Stage 0’dan daha zengin syntax kullanacak; o syntax’ı Stage 1’in üreteceği C’ye çevirmek gerekiyor. |

**Özet**: Self-hosting’e **yaklaşık %40–50** yol alınmış sayılır: Stage 0 sağlam, compiler .zn ağacının büyük kısmı parse oluyor. Eksik olan: (1) Kalan parse edge case’leri (match/check vb.), (2) Codegen’in tüm AST node’ları ve stdlib’i (Vec, Option, Result, match, trait) C’ye doğru dökmesi, (3) Stage 1 binary’nin “kendini derleyecek” minimal compiler .zn seti ile test edilmesi, (4) Bu minimal setin Stage 1 ile baştan sona derlenip aynı binary’yi üretmesi (bootstrap döngüsü).

---

## Önerilen sıradaki adımlar

1. **lexer.zn hatasını gidermek**  
   - `match expr { pat => block; ... }` için parse’ta özel “match arms” (pat, `=>`, block) akışı eklemek; blokların `}` ile kapanışında `;` veya `,` varyantlarını kabul etmek.
2. **Minimal “bootstrap subset” tanımlamak**  
   - Sadece lexer + parser + codegen + main içeren birkaç .zn ile Stage 0’ın ürettiği C’nin GCC ile sorunsuz derlendiğini doğrulamak.
3. **Codegen’i genişletmek**  
   - `match`, Option/Result, Vec, basit trait kullanımı için C çıktısını tamamlamak.
4. **Stage 1 ile bootstrap döngüsü**  
   - Stage 0 ile Stage 1 binary’yi üret; sonra bu binary’yi “kendini derleyecek” minimal .zn seti ile çalıştırıp aynı (veya eşdeğer) binary’yi üretmek.

---

## Dosya konumları

- **Stage 0**: `nova/src/native/bootstrap/nova_stage0.py` + `nova_runtime_io.c`
- **Compiler .zn**: `nova/zn/src/compiler/` (frontend, backend, bootstrap, …)
- **Frontend core (syntax referansı)**: `nova/zn/src/compiler/compiler/frontend/parser.zn` (v10 AST: data, cases, rules, open, …)
- **native_linker**: `zn/src/compiler/frontend/core/native_linker.zn`, `zn/src/tools/infra/native_linker.zn`
- **Infra stublar**: `zn/src/tools/infra/*.zn`
