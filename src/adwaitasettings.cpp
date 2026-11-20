// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitasettings.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QtMath>
#include <cmath>

namespace {
struct Named { const char *name; const char *rgb; };
// libadwaita 1.10 accent backgrounds (--accent-bg-color of each named accent)
const Named kAccents[] = {
    {"blue", "#3584e4"}, {"teal", "#2190a4"}, {"green", "#3a944a"}, {"yellow", "#c88800"},
    {"orange", "#ed5b00"}, {"red", "#e62d42"}, {"pink", "#d56199"}, {"purple", "#9141ac"},
    {"slate", "#6f8396"},
};
const char *kPortal = "org.freedesktop.portal.Desktop";
const char *kPath = "/org/freedesktop/portal/desktop";
const char *kIface = "org.freedesktop.portal.Settings";
const char *kAppearance = "org.freedesktop.appearance";
}

AdwaitaSettings *AdwaitaSettings::instance()
{
    static AdwaitaSettings *s = new AdwaitaSettings;
    return s;
}

QColor AdwaitaSettings::namedAccent(const QString &name)
{
    for (const Named &n : kAccents)
        if (name.compare(QLatin1String(n.name), Qt::CaseInsensitive) == 0)
            return QColor(QLatin1String(n.rgb));
    return QColor();
}

AdwaitaSettings::AdwaitaSettings()
{
    m_accent = namedAccent("blue");
    applyEnv();
    // A portal backend (e.g. xdg-desktop-portal-kde shows Qt file dialogs) would block on a call to itself.
    if (!QCoreApplication::applicationName().startsWith(QLatin1String("xdg-desktop-portal"))
        && !QCoreApplication::applicationFilePath().contains(QLatin1String("xdg-desktop-portal")))
        load();
    if (applyKde()) watchKde();
    QDBusConnection::sessionBus().connect(kPortal, kPath, kIface, "SettingChanged", this,
                                          SLOT(onSettingChanged(QString, QString, QDBusVariant)));
}

QVariant AdwaitaSettings::readOne(const QString &ns, const QString &key)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return {};
    // Version 2 of the portal: ReadOne returns the value directly; version 1: Read returns a variant of variant.
    QDBusMessage m = QDBusMessage::createMethodCall(kPortal, kPath, kIface, "ReadOne");
    m << ns << key;
    QDBusMessage r = bus.call(m, QDBus::Block, 400);
    if (r.type() == QDBusMessage::ReplyMessage && !r.arguments().isEmpty()) {
        QVariant v = r.arguments().first();
        if (v.canConvert<QDBusVariant>()) v = v.value<QDBusVariant>().variant();
        return v;
    }
    m = QDBusMessage::createMethodCall(kPortal, kPath, kIface, "Read");
    m << ns << key;
    r = bus.call(m, QDBus::Block, 400);
    if (r.type() == QDBusMessage::ReplyMessage && !r.arguments().isEmpty()) {
        QVariant v = r.arguments().first();
        for (int i = 0; i < 2 && v.canConvert<QDBusVariant>(); ++i) v = v.value<QDBusVariant>().variant();
        return v;
    }
    return {};
}

void AdwaitaSettings::load()
{
    apply(kAppearance, "color-scheme", readOne(kAppearance, "color-scheme"));
    apply(kAppearance, "accent-color", readOne(kAppearance, "accent-color"));
    apply(kAppearance, "contrast", readOne(kAppearance, "contrast"));
    // Optional: reduced motion (gsettings enable-animations, exposed by the GNOME portal backend)
    apply("org.gnome.desktop.interface", "enable-animations",
          readOne("org.gnome.desktop.interface", "enable-animations"));
}

void AdwaitaSettings::applyEnv()
{
    const QString cs = qEnvironmentVariable("ADWAITA_COLOR_SCHEME").toLower();
    if (cs == "light" || cs == "dark") { m_dark = cs == "dark"; m_envScheme = true; }
    const QString ac = qEnvironmentVariable("ADWAITA_ACCENT");
    if (!ac.isEmpty()) {
        QColor c = ac.startsWith('#') ? QColor(ac) : namedAccent(ac);
        if (c.isValid()) { m_accent = c; m_envAccent = true; }
    }
    const QString hc = qEnvironmentVariable("ADWAITA_HIGH_CONTRAST");
    if (!hc.isEmpty()) { m_highContrast = hc != "0"; m_envContrast = true; }
    const QString an = qEnvironmentVariable("ADWAITA_ANIMATIONS");
    if (!an.isEmpty()) { m_animations = an != "0"; m_envAnim = true; }
}

static bool isPlasma()
{
    return qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(':').contains(QLatin1String("KDE"), Qt::CaseInsensitive)
        || qEnvironmentVariableIsSet("KDE_FULL_SESSION");
}

// Reads [Colors:Window] BackgroundNormal, [Colors:Selection] BackgroundNormal, [General] AccentColor/ColorScheme.
bool AdwaitaSettings::applyKde()
{
    if (!isPlasma()) return false;
    if (m_kdeFile.isEmpty()) {
        // Flatpak apps with access to the host's file see it at ~/.config/kdeglobals; XDG_CONFIG_HOME first.
        const QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
        for (const QString &f : {xdg.isEmpty() ? QString() : xdg + "/kdeglobals", QDir::homePath() + "/.config/kdeglobals"})
            if (!f.isEmpty() && QFileInfo::exists(f)) { m_kdeFile = f; break; }
        if (m_kdeFile.isEmpty()) return false;
    }
    QFile file(m_kdeFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QString section, windowBg, schemeName;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.startsWith('[')) { section = line; continue; }
        const int eq = line.indexOf('=');
        if (eq < 0) continue;
        const QString key = line.left(eq).trimmed(), val = line.mid(eq + 1).trimmed();
        if (section == "[Colors:Window]" && key == "BackgroundNormal") windowBg = val;
        else if (section == "[General]" && key == "ColorScheme") schemeName = val;
    }
    auto rgb = [](const QString &v) {
        const QStringList p = v.split(',');
        return p.size() >= 3 ? QColor(p[0].toInt(), p[1].toInt(), p[2].toInt()) : QColor(v);
    };
    const QColor bg = rgb(windowBg);
    if (!bg.isValid()) return false;  // no colour scheme written yet: keep the portal
    m_kdeScheme = true;
    if (!m_envScheme) m_dark = qGray(bg.rgb()) < 128;
    if (!m_envContrast) m_highContrast = schemeName.contains(QLatin1String("HighContrast"), Qt::CaseInsensitive);
    return true;
}

void AdwaitaSettings::watchKde()
{
    // KConfig saves by writing a new file and renaming it over the old one, which drops the watch: re-add it.
    auto *w = new QFileSystemWatcher(this);
    w->addPath(m_kdeFile);
    w->addPath(QFileInfo(m_kdeFile).absolutePath());
    auto *debounce = new QTimer(this);
    debounce->setSingleShot(true);
    debounce->setInterval(150);
    connect(w, &QFileSystemWatcher::fileChanged, debounce, qOverload<>(&QTimer::start));
    connect(w, &QFileSystemWatcher::directoryChanged, debounce, qOverload<>(&QTimer::start));
    connect(debounce, &QTimer::timeout, this, [this, w] {
        if (!w->files().contains(m_kdeFile) && QFileInfo::exists(m_kdeFile)) w->addPath(m_kdeFile);
        const bool d = m_dark, h = m_highContrast;
        applyKde();
        if (d != m_dark || h != m_highContrast) Q_EMIT changed();
    });
}

void AdwaitaSettings::apply(const QString &ns, const QString &key, const QVariant &value)
{
    if (!value.isValid()) return;
    if (ns == kAppearance && key == "color-scheme" && !m_envScheme && !m_kdeScheme) {
        const uint v = value.toUInt();
        // 0 = no preference (stay light), 1 = prefer dark, 2 = prefer light
        m_dark = v == 1;
    } else if (ns == kAppearance && key == "contrast" && !m_envContrast && !m_kdeScheme) {
        m_highContrast = value.toUInt() == 1;
    } else if (ns == kAppearance && key == "accent-color" && !m_envAccent) {
        if (!value.canConvert<QDBusArgument>()) return;
        const QDBusArgument arg = value.value<QDBusArgument>();
        double r = -1, g = -1, b = -1;
        arg.beginStructure(); arg >> r >> g >> b; arg.endStructure();
        if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1) return;  // out of range = no accent set
        // libadwaita maps the portal colour to the nearest named accent
        double best = 1e9; QColor pick = m_accent;
        for (const Named &n : kAccents) {
            QColor c(QLatin1String(n.rgb));
            double d = std::pow(c.redF() - r, 2) + std::pow(c.greenF() - g, 2) + std::pow(c.blueF() - b, 2);
            if (d < best) { best = d; pick = c; }
        }
        m_accent = pick;
    } else if (key == "enable-animations" && !m_envAnim) {
        m_animations = value.toBool();
    }
}

void AdwaitaSettings::onSettingChanged(const QString &ns, const QString &key, const QDBusVariant &value)
{
    if (ns != kAppearance && ns != "org.gnome.desktop.interface") return;
    const bool d = m_dark, h = m_highContrast, a = m_animations; const QColor ac = m_accent;
    apply(ns, key, value.variant());
    if (d != m_dark || h != m_highContrast || a != m_animations || ac != m_accent)
        Q_EMIT changed();
}
