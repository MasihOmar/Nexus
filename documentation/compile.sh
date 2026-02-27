#!/bin/bash
# NexOS Dokümantasyon Derleme Betiği
# Bu betik LaTeX dosyasını PDF'e dönüştürür

# Renk tanımları
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${YELLOW}NexOS Dokümantasyon Derleniyor...${NC}"
echo ""

# LaTeX derleyici kontrolü
if ! command -v pdflatex &> /dev/null; then
    echo -e "${RED}HATA: pdflatex bulunamadı!${NC}"
    echo ""
    echo "Lütfen LaTeX'i kurun:"
    echo "  Ubuntu/Debian: sudo apt install texlive-full"
    echo "  macOS: brew install --cask mactex"
    echo ""
    exit 1
fi

# Dizin kontrolü
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# İlk derleme (TOC oluşturur)
echo -e "${YELLOW}[1/2] İlk derleme (İçindekiler için)...${NC}"
pdflatex -interaction=nonstopmode main.tex > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}Derleme hatası! Detaylar için main.log dosyasına bakın.${NC}"
    exit 1
fi

# İkinci derleme (Referansları çözer)
echo -e "${YELLOW}[2/2] İkinci derleme (Referanslar için)...${NC}"
pdflatex -interaction=nonstopmode main.tex > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}Derleme hatası!${NC}"
    exit 1
fi

# Temizlik (geçici dosyalar)
echo -e "${YELLOW}Geçici dosyalar temizleniyor...${NC}"
rm -f *.aux *.log *.toc *.out *.lof *.lot 2>/dev/null

# Sonuç
if [ -f "main.pdf" ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}BAŞARILI: main.pdf oluşturuldu!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Dosya konumu: $SCRIPT_DIR/main.pdf"
    echo ""
    
    # macOS'ta otomatik aç
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "PDF açılıyor..."
        open main.pdf
    fi
else
    echo -e "${RED}HATA: PDF oluşturulamadı!${NC}"
    exit 1
fi
