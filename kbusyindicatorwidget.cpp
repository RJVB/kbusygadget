/*
    Copyright 2019 Harald Sitter <sitter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "kbusyindicatorwidget.h"

#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QResizeEvent>
#include <QStyle>
#include <QVariantAnimation>

#include <QThread>

#include <QDebug>

class KBusyIndicatorWidgetPrivate
{
public:
    KBusyIndicatorWidgetPrivate(KBusyIndicatorWidget *parent)
        : q(parent)
    {
        animation.setLoopCount(-1);
        animation.setDuration(2000);
        animation.setStartValue(0);
        animation.setEndValue(360);
        QObject::connect(&animation, &QVariantAnimation::valueChanged,
                q, [=](QVariant value) {
            rotation = value.toReal();
            q->update(); // repaint new rotation
            if (freezeDuration) {
                animation.thread()->msleep(freezeDuration);
            }
        });
    }

    KBusyIndicatorWidget *q = nullptr;
    QVariantAnimation animation;
    QIcon fixedIcon = QIcon(QStringLiteral(":icons/view-refresh-fixed"));
    QIcon scalableIcon = QIcon(QStringLiteral(":icons/view-refresh-scalable"));
    qreal rotation = 0;
    QPointF paintCenter;
    bool scalable = false;
    int freezeDuration = 0;
    bool bogus = false;
};

KBusyIndicatorWidget::KBusyIndicatorWidget(QWidget *parent)
    : QWidget(parent)
    , d(new KBusyIndicatorWidgetPrivate(this))
{
}

KBusyIndicatorWidget::KBusyIndicatorWidget(bool scalable, QWidget *parent)
    : QWidget(parent)
    , d(new KBusyIndicatorWidgetPrivate(this))
{
    d->scalable = scalable;
}

KBusyIndicatorWidget::KBusyIndicatorWidget(bool scalable, int freezeMs, QWidget *parent)
    : QWidget(parent)
    , d(new KBusyIndicatorWidgetPrivate(this))
{
    d->scalable = scalable;
    d->freezeDuration = freezeMs;
}

KBusyIndicatorWidget::~KBusyIndicatorWidget()
{
    delete d;
}

QSize KBusyIndicatorWidget::minimumSizeHint() const
{
    const auto extent = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
    return QSize(extent, extent);
}

void KBusyIndicatorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    d->animation.start();
}

void KBusyIndicatorWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    d->animation.pause();
}

void KBusyIndicatorWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    d->paintCenter = QPointF(event->size().width() / 2.0,
                             event->size().height() / 2.0);
}

void KBusyIndicatorWidget::paintEvent(QPaintEvent *)
{
    if (Q_UNLIKELY(d->bogus)) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Rotate around the center and then reset back to origin for icon painting.
    painter.translate(d->paintCenter);
    painter.rotate(d->rotation);
    painter.translate(-d->paintCenter);

    if (d->scalable) {
        d->scalableIcon.paint(&painter, rect());
    } else {
        d->fixedIcon.paint(&painter, rect());
    }
}

bool KBusyIndicatorWidget::event(QEvent *event)
{
    // Only overridden to be flexible WRT binary compatible in the future.
    // Overriding later has potential to change the call going through
    // the vtable or not.
    return QWidget::event(event);
}

void KBusyIndicatorWidget::useScalable(bool enabled)
{
    d->scalable = enabled;
}

bool KBusyIndicatorWidget::scalable()
{
    return d->scalable;
}

void KBusyIndicatorWidget::setFreezeDuration(int ms)
{
    d->freezeDuration = ms >= 0 ? ms : 0;
}

int KBusyIndicatorWidget::freezeDuration()
{
    return d->freezeDuration;
}

void KBusyIndicatorWidget::setBogus(bool enabled)
{
    d->bogus = enabled;
}

bool KBusyIndicatorWidget::bogus()
{
    return d->bogus;
}
