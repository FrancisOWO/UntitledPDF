#include "tools.h"

#include <QDebug>
#include <podofo/podofo.h>

using namespace PoDoFo;
using namespace std;

const char* GetBase14FontName(unsigned i);
void DemoBase14Fonts(PdfPainter& painter, PdfPage& page, PdfDocument& document);

void PoDoFoHelloworld(std::string outputfile)
{
    PdfMemDocument document;
    PdfPainter painter;

    try {
        // 创建 helloworld.pdf
        auto& page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
        painter.SetCanvas(page);

        string fontName = "Arial";
        PdfFont* font = document.GetFonts().SearchFont(fontName);

        // font == nullptr 则使用默认字体
        painter.TextState.SetFont(*font, 18);
        painter.DrawText("ABCDEFGHIKLMNOPQRSTVXYZ", 56.69, page.GetRect().Height - 56.69);

        try {
            painter.DrawText("АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЬЫЭЮЯ", 56.69, page.GetRect().Height - 80);
        }
        catch (PdfError& e) {
            if (e.GetCode() == PdfErrorCode::InvalidFontData) {
                qDebug() << "WARNING: The matched font" << QString::fromStdString(fontName) << "doesn't support cyrillic";
            }
        }
        painter.FinishDrawing();
        document.Save(outputfile);
    }
    catch (PdfError& e) {
        try {
            painter.FinishDrawing();
        }
        catch (...) {

        }
        throw e;
    }
}

void PoDoFoBase14Fonts(std::string outputfile)
{
    PdfMemDocument document;
    PdfPainter painter;

    try {
        // 创建 helloworld.pdf
        auto& page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
        painter.SetCanvas(page);

        string fontName = "Arial";
        PdfFont* font = document.GetFonts().SearchFont(fontName);

        // font == nullptr 则使用默认字体
        painter.TextState.SetFont(*font, 18);
        painter.DrawText("Hello World!", 56.69, page.GetRect().Height - 56.69);

        // 基础字体展示
        DemoBase14Fonts(painter, page, document);

        painter.FinishDrawing();
        document.Save(outputfile);
    }
    catch (PdfError& e) {
        try {
            painter.FinishDrawing();
        }
        catch (...) {

        }
        throw e;
    }
}

static const char* s_base14fonts[] =
    {
        "Times-Roman",
        "Times-Italic",
        "Times-Bold",
        "Times-BoldItalic",
        "Helvetica",
        "Helvetica-Oblique",
        "Helvetica-Bold",
        "Helvetica-BoldOblique",
        "Courier",
        "Courier-Oblique",
        "Courier-Bold",
        "Courier-BoldOblique",
        "Symbol",
        "ZapfDingbats",
        "Arial",
        "Verdana"
};

const char* GetBase14FontName(unsigned i)
{
    if (i >= std::size(s_base14fonts))
        return nullptr;

    return s_base14fonts[i];
}

void DrawRedFrame(PdfPainter& painter, double x, double y, double width, double height)
{
    // 绘制红色方框
    painter.GraphicsState.SetFillColor(PdfColor(1.0f, 0.0f, 0.0f));
    painter.GraphicsState.SetStrokeColor(PdfColor(1.0f, 0.0f, 0.0f));
    painter.DrawLine(x, y, x + width, y);
    if (height > 0.0f) {
        painter.DrawLine(x, y, x, y + height);
        painter.DrawLine(x + width, y, x + width, y + height);
        painter.DrawLine(x, y + height, x + width, y + height);
    }
    // 恢复默认颜色
    painter.GraphicsState.SetFillColor(PdfColor(0.0f, 0.0f, 0.0f));
    painter.GraphicsState.SetStrokeColor(PdfColor(0.0f, 0.0f, 0.0f));
}

void DemoBase14Fonts(PdfPainter& painter, PdfPage& page, PdfDocument& document)
{
    PdfFontSearchParams params;
    params.AutoSelect = PdfFontAutoSelectBehavior::Standard14;

    double x = 56, y = page.GetRect().Height - 56.69;
    string_view demo_text = "abcdefgABCDEFG12345!#$%&+-@?        ";
    double height = 0.0f, width = 0.0f;

    // 依次展示每种基础字体
    for (unsigned i = 0; i < std::size(s_base14fonts); i++) {
        x = 56; y = y - 25;
        string text;
        if (i == 12) {
            // 字体 Symbol，打印 u8"♠♣♥♦"
            text = u8"\u2660\u2663\u2665\u2666";
        }
        else if (i == 13) {
            // 字体 ZapfDingbats，打印 u8"❏❑▲▼"
            text = u8"\u274f\u2751\u25b2\u25bc";
        }
        else {
            // 其他字体，打印默认文本
            text = demo_text;
            text.append(GetBase14FontName(i));
        }

        PdfFont* font = document.GetFonts().SearchFont(GetBase14FontName(i), params);
        if (font == nullptr)
            throw runtime_error("Font not found");

        painter.TextState.SetFont(*font, 12.0);

        width = font->GetStringLength(text, painter.TextState);
        height = font->GetLineSpacing(painter.TextState);

        qDebug() << GetBase14FontName(i) << "Width =" << width << "Height =" << height << "\n";

        // 文本外侧用方框包围
        DrawRedFrame(painter, x, y, width, height);
        painter.DrawText(text, x, y);
    }

    // 每行显示 text2 中的一个字符，第一个空格对应的行留给标题
    string_view demo_text2 = " @_1jiPlg .;";

    auto helveticaStd14 = document.GetFonts().SearchFont("Helvetica", params);
    auto arialImported = document.GetFonts().SearchFont("Arial");

    for (unsigned i = 0; i < demo_text2.length(); i++) {
        x = 56; y = y - 25;
        string text = (i == 0 ? "Helvetica / Arial Comparison:" : (string)demo_text2.substr(i, 1));

        painter.TextState.SetFont(*helveticaStd14, 12);
        height = helveticaStd14->GetLineSpacing(painter.TextState);
        width = helveticaStd14->GetStringLength(text, painter.TextState);

        // 文本加框
        DrawRedFrame(painter, x, y, width, height);
        painter.DrawText(text, x, y);

        if (i > 0) {
            painter.TextState.SetFont(*arialImported, 12);
            height = arialImported->GetLineSpacing(painter.TextState);
            width = arialImported->GetStringLength((string_view)text, painter.TextState);

            // 文本加框
            DrawRedFrame(painter, x + 100, y, width, height);
            painter.DrawText(text, x + 100, y);
        }
    }
}
