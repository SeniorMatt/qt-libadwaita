// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QColor>
#include <QObject>
#include <QDBusVariant>

// Reads the GNOME appearance settings from xdg-desktop-portal and follows changes live.
// Env overrides (testing): ADWAITA_COLOR_SCHEME=light|dark, ADWAITA_ACCENT=blue|...|#rrggbb,
// ADWAITA_HIGH_CONTRAST=0|1, ADWAITA_ANIMATIONS=0|1.
// On Plasma the KDE colour scheme (kdeglobals) decides light/dark and high contrast, so the style always matches
// the parts KDE apps paint themselves; the portal is used elsewhere (and for the accent everywhere).
class AdwaitaSettings : public QObject
{
    Q_OBJECT
public:
    static AdwaitaSettings *instance();

    bool dark() const { return m_dark; }
    bool highContrast() const { return m_highContrast; }
    bool animationsEnabled() const { return m_animations; }
    QColor accent() const { return m_accent; }  // libadwaita accent background colour
    static QColor namedAccent(const QString &name);  // invalid if unknown

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    void onSettingChanged(const QString &ns, const QString &key, const QDBusVariant &value);

private:
    AdwaitaSettings();
    void load();
    void apply(const QString &ns, const QString &key, const QVariant &value);
    void applyEnv();
    bool applyKde();   // Plasma: follow the KDE colour scheme in kdeglobals (what KDE apps paint with)
    void watchKde();
    QVariant readOne(const QString &ns, const QString &key);

    bool m_dark = false, m_highContrast = false, m_animations = true;
    QColor m_accent;
    QString m_kdeFile;    // kdeglobals in use (Plasma session only)
    bool m_kdeScheme = false;  // kdeglobals decides light/dark and high contrast; portal values ignored
    bool m_envScheme = false, m_envAccent = false, m_envContrast = false, m_envAnim = false;
};
