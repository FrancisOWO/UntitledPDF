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

#include "zoomselector.h"

#include <QLineEdit>
#include <QListView>
#include <QDebug>

ZoomSelector::ZoomSelector(QWidget *parent)
    : QComboBox(parent)
{
    setEditable(true);

    // FIX: 当输入的选项不存在时，不要在下拉列表中新增该选项
    setInsertPolicy(NoInsert);
    m_itemList = QStringList {
        "Undefined",    // invisible, used by undefined inputs
        "Fit Width", "Fit Page",
        "12%", "25%", "33%", "50%", "66%", "75%",
        "100%", "125%", "150%", "200%", "400%"
    };
    m_undefinedZoomIndex = 0;   // Undefined
    m_defaultZoomIndex = 9;     // 100%
    addItems(m_itemList);

    m_lastText = m_itemList[m_defaultZoomIndex];
    m_zoomFactor = 1.0;

    // 隐藏 "Undefined" 选项
    QListView *pview = qobject_cast<QListView *>(this->view());
    pview->setRowHidden(m_undefinedZoomIndex, true);

    connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ZoomSelector::onCurrentIndexChanged);
    connect(lineEdit(), &QLineEdit::editingFinished, this, [this](){
        // 如果输入的数字缺少 "%"，下面的函数会补全（如："5"->"5%"），所以下面两次调用 text() 的结果可能不同
        onCurrentTextChanged(lineEdit()->text());
        // 如果输入的选项不存在，将 Undefined 选项的 text 替换成当前输入，并设置当前选项为 Undefined，
        // 以保证下次选择正常选项时能够触发 currentIndexChanged 信号
        // 如：当前选项为 9 ("100%")，输入 "5%" 不存在对应的选项，切换到选项 0 ("5%")，缩放变为 5%，
        //    再次点击选项 9，index 改变，缩放变回 100%。如果没有切换到选项 0，缩放变为 5%，但 index 还是 9，
        //    再次点击选项 9，index 不变，缩放无法立即变回 100%，不过 lineEdit 内容已经变为 "100%"，
        //    在按回车键或者点击界面其他位置（触发 editingFinished 信号）之后，缩放变回 100%
        QString currentText = lineEdit()->text();
        if (m_itemList[m_undefinedZoomIndex] == currentText || !m_itemList.contains(currentText)) {
            setItemText(m_undefinedZoomIndex, currentText);
            setCurrentIndex(m_undefinedZoomIndex);
        }
    });
}

void ZoomSelector::setZoomFactor(qreal zoomFactor)
{
    m_lastText = QString::fromLatin1("%1%").arg(qRound(zoomFactor * 100));
    setCurrentText(m_lastText);
}

void ZoomSelector::reset()
{
    setCurrentIndex(m_defaultZoomIndex); // 100%
}

void ZoomSelector::onCurrentIndexChanged(int index)
{
    qDebug() << "onCurrentIndexChanged()" << index;
    onCurrentTextChanged(this->itemText(index));
}

void ZoomSelector::onCurrentTextChanged(const QString &text)
{
    if (text == m_lastText)
        return;
    m_lastText = text;

    qDebug() << "onCurrentTextChanged()" << text;
    if (text == QLatin1String("Fit Width")) {
        emit zoomModeChanged(QPdfView::FitToWidth);
    } else if (text == QLatin1String("Fit Page")) {
        emit zoomModeChanged(QPdfView::FitInView);
    } else {
        QString withoutPercent(text);
        withoutPercent.remove(QLatin1Char('%'));

        bool ok = false;
        const int zoomLevel = withoutPercent.toInt(&ok);
        if (ok) {
            m_zoomFactor = zoomLevel / 100.0;
            emit zoomModeChanged(QPdfView::CustomZoom);
            // 第一次输入 "5"，补全为 "5%"，再输入 "5"，接收信号的 pdfView 认为 zoomFactor 没有变化，
            // 不会发送信号给 ZoomSelector，则不会补全 "%"，所以下面需要显式调用 setZoomFactor 来补全 "%"
            emit zoomFactorChanged(m_zoomFactor);
        }
        // 如果当前输入是数字，在 lineEdit 中正常显示，缺少 % 则补全
        // 如果当前输入不合法，将 lineEdit 的内容恢复为上次的缩放比例（factor*100%）
        setZoomFactor(m_zoomFactor);
    }
}
