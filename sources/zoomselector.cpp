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
#include <QStringList>

ZoomSelector::ZoomSelector(QWidget *parent)
    : QComboBox(parent)
{
    setEditable(true);

    QStringList itemList = {
        "Fit Width", "Fit Page",
        "12%", "25%", "33%", "50%", "66%", "75%",
        "100%", "125%", "150%", "200%", "400%"
    };
    addItems(itemList);
    m_defaultZoomIndex = 8; // 100%

    connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ZoomSelector::onCurrentIndexChanged);
}

void ZoomSelector::setZoomFactor(qreal zoomFactor)
{
    setCurrentText(QString::fromLatin1("%1%").arg(qRound(zoomFactor * 100)));
}

void ZoomSelector::reset()
{
    setCurrentIndex(m_defaultZoomIndex); // 100%
}

void ZoomSelector::onCurrentIndexChanged(int index)
{
    onCurrentTextChanged(this->itemText(index));
}

void ZoomSelector::onCurrentTextChanged(const QString &text)
{
    if (text == QLatin1String("Fit Width")) {
        emit zoomModeChanged(QPdfView::FitToWidth);
    } else if (text == QLatin1String("Fit Page")) {
        emit zoomModeChanged(QPdfView::FitInView);
    } else {
        qreal factor = 1.0;

        QString withoutPercent(text);
        withoutPercent.remove(QLatin1Char('%'));

        bool ok = false;
        const int zoomLevel = withoutPercent.toInt(&ok);
        if (ok) {
            factor = zoomLevel / 100.0;
            emit zoomModeChanged(QPdfView::CustomZoom);
            emit zoomFactorChanged(factor);
        }
    }
}
