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
#include <QTimer>
#include <QElapsedTimer>

#include <QThread>

#include <QDebug>

class KBusyIndicatorWidgetPrivate
{
public:
    KBusyIndicatorWidgetPrivate(KBusyIndicatorWidget *parent)
        : q(parent)
    {
        // duration in milliseconds:
        const int durationMs = 2000;
        animation.setLoopCount(-1);
        animation.setDuration(durationMs);
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

        if (freezeDuration) {
            aniTimer.setInterval(freezeDuration);
        } else {
            aniTimer.setInterval(1000/frequency);
        }
        QObject::connect(&aniTimer, &QTimer::timeout,
                q, [=]() {
            q->update(); // repaint new rotation
            // use the actual dt instead of the programmed dt
            // the conversion from ms to s is implicit through
            // the unit of durationMs;
            auto elapsed = aniTime.restart();
            rotation += elapsed * 360.0 / durationMs;
            if (rotation > 360) {
                rotation -= 360;
                qWarning() << "full turn after t=" << test.restart() / 1000.0;
            }
        });

    }

    KBusyIndicatorWidget *q = nullptr;
    bool running = false;
    QVariantAnimation animation;
    QIcon fixedIcon = QIcon(QStringLiteral(":icons/view-refresh-fixed"));
    QIcon scalableIcon = QIcon(QStringLiteral(":icons/view-refresh-scalable"));
    qreal rotation = 0;
    QPointF paintCenter;
    bool scalable = false;
    int freezeDuration = 0;
    bool bogus = false;
    QTimer aniTimer;
    int frequency = 60;
    QElapsedTimer aniTime, test;
    bool useInternalTimer = false;
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
    if (d->useInternalTimer) {
        d->aniTimer.start();
    } else {
        d->animation.start();
    }
    d->aniTime.start();
    d->test.start();
    d->running = true;
}

void KBusyIndicatorWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if (d->useInternalTimer) {
        d->aniTimer.stop();
    } else {
        d->animation.pause();
    }
    d->aniTime.invalidate();
    d->running = false;
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

void KBusyIndicatorWidget::setUseInternalTimer(bool enabled)
{
    if (enabled != d->useInternalTimer && d->running) {
        // just stop both
        d->aniTimer.stop();
        d->animation.stop();
        if (enabled) {
            d->aniTimer.start();
        } else {
            d->animation.start();
        }
    }
    d->useInternalTimer = enabled;
}

bool KBusyIndicatorWidget::useInternalTimer()
{
    return d->useInternalTimer;
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
    if (d->freezeDuration) {
        d->aniTimer.setInterval(d->freezeDuration);
    } else {
        d->aniTimer.setInterval(1000/60);
    }
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
