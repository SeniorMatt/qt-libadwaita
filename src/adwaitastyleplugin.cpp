// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyleplugin.h"
#include "adwaitastyle.h"
#include <QCoreApplication>
#include <QFileInfo>

// KWin is a Qt application too and loads the configured widget style. A blocking portal call or any fault in
// the style there freezes or kills the whole session, and the compositor has no widgets worth styling: refuse,
// so Qt falls back to the next style (Breeze/Fusion).
static bool unsafeHost()
{
    if (!QCoreApplication::instance()) return false;
    const QString exe = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    return exe.startsWith(QLatin1String("kwin")) || QCoreApplication::applicationName().startsWith(QLatin1String("kwin"));
}

QStyle *AdwaitaStylePlugin::create(const QString &key)
{
    if (key.compare(QLatin1String("libadwaita"), Qt::CaseInsensitive) != 0 || unsafeHost()) return nullptr;
    return new AdwaitaStyle;
}
