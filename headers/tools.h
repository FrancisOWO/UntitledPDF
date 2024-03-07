#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QFont>

#include <vector>
#include <string>
#include <podofo/podofo.h>

struct UPdfTextState {
    const PoDoFo::PdfFont* font = nullptr;
    double fontSize = -1;
    double fontScale = 1;
    double charSpacing = 0;
    double wordSpacing = 0;
};

void PdfFont2QFont(const QString& baseFontName, QString& to_fontName, QFont::StyleHint& to_hint,
                   QFont::Style& to_style, QFont::Weight& to_weight);
void QFont2PdfFont(const QFont& font, QString& to_fontName);

void PoDoFoHelloworld(std::string outputfile);
void PoDoFoBase14Fonts(std::string outputfile);

void UPdfExtractTextStates(PoDoFo::PdfPage& page, std::vector<UPdfTextState>& textStates);

#endif // TOOLS_H
