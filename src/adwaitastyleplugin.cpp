// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyleplugin.h"
#include "adwaitastyle.h"

QStyle *AdwaitaStylePlugin::create(const QString &key)
{
    return key.compare(QLatin1String("libadwaita"), Qt::CaseInsensitive) == 0 ? new AdwaitaStyle : nullptr;
}
