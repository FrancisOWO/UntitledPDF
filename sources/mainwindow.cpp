/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtPDF module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "pageselector.h"
#include "zoomselector.h"
#include "tools.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfPageNavigation>
#include <QtMath>

#include <QScreen>

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QFont>
#include <QLayout>

#include <QDir>
#include <QDebug>

#include <podofo/podofo.h>

using namespace PoDoFo;

const qreal zoomMultiplier = qSqrt(2.0);

Q_LOGGING_CATEGORY(lcExample, "qt.examples.pdfviewer")

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_zoomSelector(new ZoomSelector(this))
    , m_pageSelector(new PageSelector(this))
    , m_document(new QPdfDocument(this))
{
    ui->setupUi(this);

    // zoomSelector
    m_zoomSelector->setMaximumWidth(150);
    ui->mainToolBar->insertWidget(ui->actionZoom_In, m_zoomSelector);

    connect(m_zoomSelector, &ZoomSelector::zoomModeChanged, ui->pdfView, &QPdfView::setZoomMode);
    connect(m_zoomSelector, &ZoomSelector::zoomFactorChanged, ui->pdfView, &QPdfView::setZoomFactor);
    m_zoomSelector->reset();

    // pageSelector
    m_pageSelector->setMaximumWidth(150);
    ui->mainToolBar->addWidget(m_pageSelector);

    m_pageSelector->setPageNavigation(ui->pdfView->pageNavigation());

    // bookmark
    QPdfBookmarkModel *bookmarkModel = new QPdfBookmarkModel(this);
    bookmarkModel->setDocument(m_document);

    ui->bookmarkView->setModel(bookmarkModel);
    // Click bookmark -> Jump to page
    connect(ui->bookmarkView, SIGNAL(activated(QModelIndex)), this, SLOT(bookmarkSelected(QModelIndex)));

    // tabWidget
    ui->tabWidget->setTabEnabled(1, false); // disable 'Pages' tab for now

    // pdfView
    ui->pdfView->setDocument(m_document);
    connect(ui->pdfView, &QPdfView::zoomFactorChanged, m_zoomSelector, &ZoomSelector::setZoomFactor);
}

MainWindow::~MainWindow()
{
    delete ui;
}

static int Pt2Px(double pt, QWidget* widget, int choice)
{
    int dpi = (choice == 0 ? widget->logicalDpiX(): widget->logicalDpiY());
    qDebug() << "dpi:" << dpi << (choice == 0 ? "X" : "Y");
    return pt/72*dpi;
}

void MainWindow::PoDoFoDemo(int choice)
{
    try {
        // 创建 helloworld.pdf
        QString outputfile;
        if (choice == DEMO_HELLOWORLD) {
            outputfile = "helloworld.pdf";
            PoDoFoHelloworld(outputfile.toStdString());
        } else if (choice == DEMO_BASE14FONTS) {
            outputfile = "helloworld-base14.pdf";
            PoDoFoBase14Fonts(outputfile.toStdString());
        } else {
            // 没有对应的 Demo
            return;
        }
        // 打开 helloworld.pdf
        auto reply = QMessageBox::question(
            this, tr("Open output file"),
            tr("PDF file helloworld.pdf created successfully.\n"
               "Do you want to open it?"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QUrl toOpen = QUrl::fromLocalFile(QString("%1/%2").arg(QDir::currentPath(), outputfile));
            open(toOpen);
        }
    }
    catch (...) {

    }
}

void MainWindow::setEditablePageSize(double width, double height)
{
    // 设置页面大小，水平居中
    ui->pdfPage->setFixedSize(::Pt2Px(width, ui->pdfPage, 0), ::Pt2Px(height, ui->pdfPage, 1));
    QVBoxLayout layout;
    layout.setAlignment(ui->pdfPage, Qt::AlignHCenter);
}

void MainWindow::loadEditablePDF()
{
    qDebug() << "loadEditablePDF() >> dpi:" << this->screen()->logicalDotsPerInch();
    // TODO: 没有切换文件则无需重新解析，继续使用上一次的编辑框
    // 清除上一次的编辑框
    for (auto& textEdit : m_textEdits) {
        textEdit->hide();
        delete textEdit;
    }
    m_textEdits.clear();
    if (m_docLocation.isLocalFile()) {
        PdfMemDocument document;
        try {
            qDebug() << m_docLocation.toLocalFile();
            document.Load(m_docLocation.toLocalFile().toStdString());

            // TODO: 目前编辑状态只显示第一页，未实现页面切换
            // pageIndex = pageNumber - 1
            auto& page = document.GetPages().GetPageAt(m_pageSelector->getPageNumber()-1);

            // TrimBox 定义了页面最终的尺寸
            auto&& trimBox = page.GetTrimBox();
            double width = trimBox.GetRight()-trimBox.GetLeft();
            double height = trimBox.GetTop()-trimBox.GetBottom();
            setEditablePageSize(width, height);

            // 提取文本内容和位置
            std::vector<PdfTextEntry> entries;
            page.ExtractTextTo(entries);

            // 提取文本字体状态
            std::vector<UPdfTextState> textStates;
            UPdfExtractTextStates(page, textStates);

            for (int i=0; i<entries.size(); i++) {
                auto& entry = entries[i];
                auto& currentState = textStates[i];

                qDebug() << QString("(%1,%2) %3 %4")
                                .arg(QString::number(entry.X), QString::number(entry.Y),
                                     QString::number(entry.Length), entry.Text.data());

                // e.g. baseFontName: "Times", fontName: "Times-BoldItalic"
                QString baseFontName = currentState.font->GetMetrics().GetBaseFontName().data();
                QString fontName = currentState.font->GetMetrics().GetFontName().data();

                qDebug() << "baseFontName:" << baseFontName << "fontName:" << fontName
                         << "fontSize:" << currentState.fontSize;

                QTextEdit *textEdit = new QTextEdit();
                textEdit->setParent(ui->pdfPage);

                // 设置字体格式
                QFont::StyleHint fontHint = QFont::System;
                QFont::Style fontStyle = QFont::StyleNormal;
                QFont::Weight fontWeight = QFont::Weight::Normal;
                PdfFont2QFont(baseFontName, fontName, fontHint, fontStyle, fontWeight);

                QFont currentFont(fontName, currentState.fontSize);
                currentFont.setStyle(fontStyle);
                currentFont.setWeight(fontWeight);
                currentFont.setStyleHint(fontHint);

                textEdit->setCurrentFont(currentFont);
                textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

                currentFont = textEdit->currentFont();

                // 文本位置坐标转换成像素位置，在编辑区域生成可编辑的文本框
                // 1. 计算宽高，需测量当前字体字符的宽高
                QFontMetrics fm(currentFont);
                int width = fm.horizontalAdvance(entry.Text.data()), height = fm.height();
                const int horizontalMargin = 15, verticalMargin = 12;

                // 2. 计算左上角坐标
                // BUG: 编辑模式显示的文本位置比正常位置偏下
                // FIX: 文本 entry.Y 是左下角的 Y，需加上字体高度才能得到左上角的 Y
                auto&& trimBox = page.GetTrimBox();
                int x = ::Pt2Px(entry.X-trimBox.GetLeft(), ui->pdfPage, 0);
                int y = ::Pt2Px(trimBox.GetTop()-entry.Y, ui->pdfPage, 1) - height;

                textEdit->setGeometry({x, y, width+horizontalMargin, height+verticalMargin});
                qDebug() << "Rect:" << x << "," << y << "," << width << "," << height;

                textEdit->append(entry.Text.data());
                textEdit->show();
                qDebug() << "Content size:" << textEdit->document()->size();

                // 文本框大小自适应，宽度高度随着内容改变
                connect(textEdit, &QTextEdit::textChanged, textEdit, [=](){
                    // 计算最长的一行文本宽度（单位：像素）
                    QString text = textEdit->toPlainText();
                    int maxWidth = 0, lastIndex = 0, lineCount = 1;
                    QFontMetrics fm(textEdit->currentFont());
                    for (int i=0; i<text.length(); i++) {
                        if (text[i] != '\n')
                            continue;
                        // 根据 font 计算文本宽度
                        int width = fm.horizontalAdvance(text.mid(lastIndex, i-lastIndex));
                        if (width > maxWidth)
                            maxWidth = width;
                        lastIndex = i;
                        lineCount++;
                    }
                    int width = fm.horizontalAdvance(text.right(text.length()-lastIndex));
                    if (width > maxWidth)
                        maxWidth = width;

                    textEdit->resize({maxWidth+horizontalMargin, fm.height()*lineCount+verticalMargin});
                });

                // 记录文本原始坐标
                m_textPositions.append({entry.X, entry.Y});
                // 保存文本框指针
                m_textEdits.append(textEdit);
            }
        }
        catch (PdfError& e) {
            // TODO: 目前不支持解析没有 xref 的 PDF，后续可以加入 xref 补全
            e.PrintErrorMsg();
            QString msg = QString::fromStdString(std::string(e.ErrorMessage(e.GetCode())));
            QMessageBox::critical(this, tr("Failed to open"), msg);
        }

    } else {
        qCDebug(lcExample) << m_docLocation << "is not a valid local file";
        // QMessageBox::critical(this, tr("Failed to open"), tr("%1 is not a valid local file").arg(m_docLocation.toString()));
    }
}

void MainWindow::open(const QUrl &docLocation)
{
    if (docLocation.isLocalFile()) {
        m_docLocation = docLocation;
        m_document->load(docLocation.toLocalFile());
        // FIX: 窗口标题应该显示文件名，而不是 PDF 元数据中的 Title
        const auto documentTitle = docLocation.fileName();
        setWindowTitle(!documentTitle.isEmpty() ? documentTitle : QStringLiteral("UntitledPDF"));
    } else {
        qCDebug(lcExample) << docLocation << "is not a valid local file";
        QMessageBox::critical(this, tr("Failed to open"), tr("%1 is not a valid local file").arg(docLocation.toString()));
    }
    qCDebug(lcExample) << docLocation;

    // 切换到阅读模式
    ui->tabWidgetTools->setCurrentWidget(ui->viewTab);
}

void MainWindow::bookmarkSelected(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const int page = index.data(QPdfBookmarkModel::PageNumberRole).toInt();
    ui->pdfView->pageNavigation()->setCurrentPage(page);
}

void MainWindow::on_actionOpen_triggered()
{
    QUrl toOpen = QFileDialog::getOpenFileUrl(this, tr("Choose a PDF"), QUrl(), "Portable Documents (*.pdf)");
    if (toOpen.isValid())
        open(toOpen);
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionSave_As_triggered()
{
    // 指定保存文件路径
    QUrl toSave = QFileDialog::getSaveFileUrl(this, tr("Save a PDF"), QUrl(), "Portable Documents (*.pdf)");
    QString outputfile = toSave.toLocalFile();
    qDebug() << "outputfile:" << outputfile;

    PdfMemDocument document;
    PdfPainter painter;

    // TODO: 目前只支持单页编辑保存，多页编辑保存待实现
    auto& page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    painter.SetCanvas(page);

    // 依次将每个文本框的内容写入 PDF 页面
    for (int i=0; i<m_textEdits.size(); i++) {
        auto& textEdit = m_textEdits[i];
        auto& pos = m_textPositions[i];

        // 获取字体
        QString fontName;
        QFont qfont = textEdit->currentFont();
        QFont2PdfFont(qfont, fontName);

        PdfFontSearchParams params;
        params.AutoSelect = PdfFontAutoSelectBehavior::Standard14;
        PdfFont* font = document.GetFonts().SearchFont(fontName.toStdString(), params);
        qDebug() << "fontName:" << fontName;

        painter.TextState.SetFont(*font, qfont.pointSize());
        painter.DrawText(textEdit->toPlainText().toStdString(), pos.x(), pos.y());
    }
    painter.FinishDrawing();
    document.Save(outputfile.toStdString());

    // 打开保存的文件
    auto reply = QMessageBox::question(
        this, tr("Open output file"),
        tr("PDF file saved successfully.\n"
           "Do you want to open it?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        open(toSave);
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About UntitledPDF"),
        tr("A PDF Editor full of unknown"));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionZoom_In_triggered()
{
    ui->pdfView->setZoomFactor(ui->pdfView->zoomFactor() * zoomMultiplier);
}

void MainWindow::on_actionZoom_Out_triggered()
{
    ui->pdfView->setZoomFactor(ui->pdfView->zoomFactor() / zoomMultiplier);
}

void MainWindow::on_actionPrevious_Page_triggered()
{
    ui->pdfView->pageNavigation()->goToPreviousPage();
}

void MainWindow::on_actionNext_Page_triggered()
{
    ui->pdfView->pageNavigation()->goToNextPage();
}

void MainWindow::on_actionContinuous_triggered()
{
    ui->pdfView->setPageMode(ui->actionContinuous->isChecked() ? QPdfView::MultiPage : QPdfView::SinglePage);
}

void MainWindow::on_actionPoDoFo_Helloworld_triggered()
{
    PoDoFoDemo(DEMO_HELLOWORLD);
}

void MainWindow::on_actionPoDoFo_Base14Fonts_triggered()
{
    PoDoFoDemo(DEMO_BASE14FONTS);
}

void MainWindow::on_tabWidgetTools_currentChanged(int index)
{
    if (ui->tabWidgetTools->currentWidget() == ui->editTab) {
        qDebug() << "Mode: Edit";
        loadEditablePDF();
    }
    else {
        qDebug() << "Mode: View";
    }
}

