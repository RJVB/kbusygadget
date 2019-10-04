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
#include <QLabel>
#include <QToolButton>
#include <QSlider>
#include <QVBoxLayout>

#include <QDebug>

#include <QCommandLineParser>
#include <QCommandLineOption>

#include <QShortcut>

class KButton : public QToolButton
{
    Q_OBJECT
public:
    KButton(const QString &text, bool checked, QWidget *parent = nullptr)
        : QToolButton(parent)
    {
        setText(text);
        setCheckable(true);
        setChecked(checked);
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    int freezeDuration = 250;
    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption freezeOption(QStringLiteral("freezeDuration"),
                                            QStringLiteral("the time (in ms) animation can be frozen between frames (must be in [0,1000])"),
                                            "freezeDuration", QStringLiteral("%1").arg(freezeDuration));
    parser.addOption(freezeOption);
    parser.process(app);

    if (parser.isSet(freezeOption)) {
        freezeDuration = parser.value(freezeOption).toInt();
        if (freezeDuration < 0) {
            qWarning() << "ignoring negative freezeDuration";
            freezeDuration = 0;
        } else if (freezeDuration > 1000) {
            freezeDuration = 1000;
        }
    }

    QWidget window;
    window.setBaseSize(128, 128);
    auto layout = new QVBoxLayout(&window);

    auto busyWidget = new QWidget(&window);
    auto busyLayout = new QHBoxLayout(busyWidget);
    auto busyIndicator = new KBusyIndicatorWidget(&window);
    auto busyLabel = new QLabel(QStringLiteral("Busy..."), &window);
    busyLayout->addWidget(busyIndicator);
    busyLayout->addWidget(busyLabel);

    auto buttons = new QHBoxLayout();
    auto toggle = new KButton(QStringLiteral("Visible"), true, &window);

    auto scalable = new KButton(QStringLiteral("Scalable"), busyIndicator->scalable(), &window);
    scalable->setToolTip(QStringLiteral("Use a scalable (svg) or fixed (png) icon"));

    auto freeze = new KButton(QStringLiteral("Freeze"), busyIndicator->freezeDuration() > 0, &window);
    freeze->setToolTip(QStringLiteral("Freeze the animation for %1ms between frames").arg(freezeDuration));
    auto fslider = new QSlider(Qt::Horizontal, &window);
    fslider->setRange(0, 1000);
    fslider->setValue(freezeDuration);

    auto bogus = new KButton(QStringLiteral("Bogus"), busyIndicator->bogus(), &window);
    bogus->setToolTip(QStringLiteral("Run bogus animation loop"));

    QObject::connect(toggle, &KButton::clicked,
            busyWidget, [=] {
        busyWidget->setVisible(!busyWidget->isVisible());
    });
    QObject::connect(scalable, &KButton::toggled,
            busyWidget, [=](bool checked) {
        busyIndicator->useScalable(checked);
    });
    QObject::connect(freeze, &KButton::toggled,
            busyWidget, [&](bool checked) {
        if (checked) {
            // somehow this assignment doesn't affect the actual variable, so get
            // the intended value from the slider widget
            freezeDuration = fslider->value();
            busyIndicator->setFreezeDuration(freezeDuration);
            qWarning() << "Freezing" << freezeDuration << "between frames";
        } else {
            busyIndicator->setFreezeDuration(0);
        }
    });
    QObject::connect(fslider, &QSlider::valueChanged,
            busyWidget, [&](int value) {
        if (value != freezeDuration) {
            // somehow this assignment doesn't affect the actual variable, so we'll
            // get the intended value from the slider widget
            freezeDuration = value;
            freeze->setToolTip(QStringLiteral("Freeze the animation for %1ms between frames").arg(freezeDuration));
            if (freeze->isChecked()) {
                busyIndicator->setFreezeDuration(freezeDuration);
                qWarning() << "Freezing" << freezeDuration << "between frames";
            } else {
                busyIndicator->setFreezeDuration(0);
            }
        }
    });
    QObject::connect(bogus, &KButton::toggled,
            busyWidget, [=](bool checked) {
        busyIndicator->setBogus(checked);
    });

    buttons->addWidget(toggle);
    buttons->addWidget(scalable);
    buttons->addWidget(freeze);
    buttons->addWidget(bogus);
    layout->addLayout(buttons);
    layout->addWidget(fslider);
    layout->addWidget(busyWidget);
    layout->setAlignment(Qt::AlignTop);

    QObject::connect(new QShortcut(QKeySequence::Quit, &window), &QShortcut::activated, &window, &QWidget::close);
    QObject::connect(new QShortcut(QKeySequence::Cancel, &window), &QShortcut::activated, &window, &QWidget::close);

    window.show();
    return app.exec();
}

#include "kbusyindicatorwidgettest.moc"
