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

void getAvailableFont(const QString& baseFontName, QString& fontName, QFont::StyleHint& hint,
                      QFont::Style& style, QFont::Weight& weight);

void PoDoFoHelloworld(std::string outputfile);
void PoDoFoBase14Fonts(std::string outputfile);

void UPdfExtractTextStates(PoDoFo::PdfPage& page, std::vector<UPdfTextState>& textStates);

#endif // TOOLS_H
