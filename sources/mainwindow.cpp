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
#include <QScrollBar>

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

int MainWindow::Pt2Px(double pt)
{
    // default dpi: 96
    double dpi = this->screen()->logicalDotsPerInch();
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
                textEdit->setParent(ui->pdfEditor);

                // 设置字体格式
                QFont::StyleHint fontHint = QFont::System;
                QFont::Style fontStyle = QFont::StyleNormal;
                QFont::Weight fontWeight = QFont::Weight::Normal;
                getAvailableFont(baseFontName, fontName, fontHint, fontStyle, fontWeight);

                QFont currentFont(fontName, currentState.fontSize);
                currentFont.setStyle(fontStyle);
                currentFont.setWeight(fontWeight);
                currentFont.setStyleHint(fontHint);

                textEdit->setCurrentFont(currentFont);
                textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

                // 转换成像素位置，显示在编辑区域
                // TODO: Pt2Px 计算的位置不够准确
                QFontMetrics fm(textEdit->currentFont());
                int x = Pt2Px(entry.X), y = Pt2Px(page.GetRect().Height-entry.Y);
                int width = fm.horizontalAdvance(entry.Text.data()), height = fm.height();
                const int horizontalMargin = 15, verticalMargin = 12;
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

                m_textEdits.append(textEdit);
            }
        }
        catch (PdfError& e) {
            throw e;
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
        setWindowTitle(!documentTitle.isEmpty() ? documentTitle : QStringLiteral("PDF Viewer"));
    } else {
        qCDebug(lcExample) << docLocation << "is not a valid local file";
        QMessageBox::critical(this, tr("Failed to open"), tr("%1 is not a valid local file").arg(docLocation.toString()));
    }
    qCDebug(lcExample) << docLocation;
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

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About PdfViewer"),
        tr("An example using QPdfDocument"));
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

