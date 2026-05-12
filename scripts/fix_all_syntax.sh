#!/bin/bash
# Nova Complete Syntax Fixer
# Tüm eski Nova syntax'ını modern C compiler syntax'ına çevirir

echo "🔧 Nova Complete Syntax Fixer"
echo "=============================="
echo ""

# Backup
echo "📦 Yedek oluşturuluyor..."
tar -czf zn_complete_backup_$(date +%Y%m%d_%H%M%S).tar.gz zn/
echo "✅ Yedek alındı"
echo ""

# Syntax dönüşümleri
echo "🔄 Syntax dönüşümleri başlıyor..."
echo ""

# 1. expose shape → expose data
echo "1/15: expose shape → expose data"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/expose shape/expose data/g' {} \;

# 2. expose trait → expose trait  
echo "2/15: expose trait → expose trait"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/expose trait/expose trait/g' {} \;

# 3. apply → impl
echo "3/15: apply → impl"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^apply /impl /g' {} \;

# 4. skill → impl (struct için)
echo "4/15: skill Type { → impl Type {"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^skill /impl /g' {} \;
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^expose skill /expose impl /g' {} \;

# 5. kind → enum
echo "5/15: kind → enum"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^expose kind/expose enum/g' {} \;
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^kind /enum /g' {} \;

# 6. cases → enum
echo "6/15: cases → enum"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^expose cases/expose enum/g' {} \;

# 7. use crate:: → use (comment yapmak daha güvenli)
echo "7/15: use crate:: → // use crate::"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^use crate::/\/\/ use crate::/g' {} \;

# 8. given → comment (global değişkenler desteklenmiyor)
echo "8/15: given → // given"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^given /\/\/ given /g' {} \;

# 9. #[cfg → // cfg (attributes desteklenmiyor)
echo "9/15: #[cfg → // cfg"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/#\[cfg/\/\/ cfg/g' {} \;

# 10. pub fields → fields (struct içinde pub desteklenmiyor)
echo "10/15: pub field → field"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^    pub /    /g' {} \;
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^        pub /        /g' {} \;

# 11. Newtype patterns: data X(Y); → data X { value: Y }
echo "11/15: newtype patterns"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^data \([A-Za-z_]*\)(\([^)]*\));$/data \1 { value: \2 }/g' {} \;
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^expose data \([A-Za-z_]*\)(\([^)]*\));$/expose data \1 { value: \2 }/g' {} \;

# 12. derives → comment (derives desteklenmiyor)
echo "12/15: derives → comment"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/ derives \[.*\]/ \/\* derives \*\//g' {} \;

# 13. open fn → pub fn
echo "13/15: open fn → pub fn"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/open fn /pub fn /g' {} \;
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/expose fn /pub fn /g' {} \;

# 14. use kernel:: → comment (internal modules)
echo "14/15: use kernel:: → comment"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/^use kernel::/\/\/ use kernel::/g' {} \;

# 15. expose open → pub
echo "15/15: expose open → pub"
find zn -name "*.zn" -type f -exec sed -i.syntaxbak 's/expose open /pub /g' {} \;

echo ""
echo "✅ Syntax dönüşümleri tamamlandı!"
echo ""

# Cleanup backup files
echo "🧹 Geçici dosyalar temizleniyor..."
find zn -name "*.syntaxbak" -delete
echo "✅ Temizlik tamamlandı"
echo ""

echo "🎉 İşlem tamamlandı!"
echo ""
echo "Test için:"
echo "  cd nova && ./build/tools/znc tests/verify_cli_capabilities.zn -o build/test.nbc --backend vm"
