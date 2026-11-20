// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QStylePlugin>

class AdwaitaStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QStyleFactoryInterface_iid FILE "adwaita.json")
public:
    QStyle *create(const QString &key) override;
};
