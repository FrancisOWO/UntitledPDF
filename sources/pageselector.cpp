/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
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

#include "pageselector.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPdfPageNavigation>
#include <QToolButton>

PageSelector::PageSelector(QWidget *parent)
    : QWidget(parent)
    , m_pageNavigation(nullptr)
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    // UI: [<] 1 / n [>]
    m_previousPageButton = new QToolButton(this);
    m_previousPageButton->setText("<");
    m_previousPageButton->setEnabled(false);

    m_pageNumberEdit = new QLineEdit(this);
    m_pageNumberEdit->setAlignment(Qt::AlignRight);

    m_pageCountLabel = new QLabel(this);
    setPageCountLabel(0);

    m_nextPageButton = new QToolButton(this);
    m_nextPageButton->setText(">");
    m_nextPageButton->setEnabled(false);

    layout->addWidget(m_previousPageButton);
    layout->addWidget(m_pageNumberEdit);
    layout->addWidget(m_pageCountLabel);
    layout->addWidget(m_nextPageButton);
}

void PageSelector::setPageNavigation(QPdfPageNavigation *pageNavigation)
{
    m_pageNavigation = pageNavigation;

    // prevPage
    connect(m_previousPageButton, &QToolButton::clicked, m_pageNavigation, &QPdfPageNavigation::goToPreviousPage);
    connect(m_pageNavigation, &QPdfPageNavigation::canGoToPreviousPageChanged, m_previousPageButton, &QToolButton::setEnabled);

    // nextPage
    connect(m_nextPageButton, &QToolButton::clicked, m_pageNavigation, &QPdfPageNavigation::goToNextPage);
    connect(m_pageNavigation, &QPdfPageNavigation::canGoToNextPageChanged, m_nextPageButton, &QToolButton::setEnabled);

    // currentPage
    connect(m_pageNavigation, &QPdfPageNavigation::currentPageChanged, this, &PageSelector::onCurrentPageChanged);
    connect(m_pageNavigation, &QPdfPageNavigation::pageCountChanged, this, [this](int pageCount){
        setPageCountLabel(pageCount);
        // FIX: 打开新文件时，当前页码应该显示 1，而不是 0
        onCurrentPageChanged(m_pageNavigation->currentPage());
    });
    connect(m_pageNumberEdit, &QLineEdit::editingFinished, this, &PageSelector::pageNumberEdited);

    // Init current page
    onCurrentPageChanged(m_pageNavigation->currentPage());
}

void PageSelector::setPageNumberEdit(int page)
{
    m_pageNumberEdit->setText(QString::number(page));
}

void PageSelector::setPageCountLabel(int pageCount)
{
    m_pageCountLabel->setText(QString::fromLatin1("/ %1").arg(pageCount));
}

void PageSelector::onCurrentPageChanged(int page)
{
    // BUG: 打开新文件时，文件状态尚未变为 Ready，导致 pageCount 置 0，页码显示为 0 而不是 1
    // FIX: 新文件 Ready 后会触发 pageCountChanged，此时 pageCount 为正常页数，再次调用当前函数即可显示正常页码
    if (m_pageNavigation->pageCount() == 0)
        setPageNumberEdit(0);
    else
        setPageNumberEdit(page + 1);
}

void PageSelector::pageNumberEdited()
{
    if (!m_pageNavigation)
        return;

    const QString text = m_pageNumberEdit->text();

    bool ok = false;
    const int pageNumber = text.toInt(&ok);

    if (!ok)
        onCurrentPageChanged(m_pageNavigation->currentPage());
    else
        m_pageNavigation->setCurrentPage(qBound(0, pageNumber - 1, m_pageNavigation->pageCount() - 1));
}
