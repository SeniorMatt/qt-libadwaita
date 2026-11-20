// SPDX-License-Identifier: GPL-2.0-or-later
// Focus ring and shared state colour helpers. (Ring drawing itself is AdwaitaStyle::drawFocusRing in adwaitastyle.cpp;
// this file holds the fill-colour logic shared by buttons, combo boxes and entries.)
#include "adwaitastyle.h"
#include <QStyleOption>

static QColor withAlphaF(QColor c, qreal a) { c.setAlphaF(c.alphaF() * a); return c; }
static QColor over(const QColor &b, const QColor &c, qreal a)
{
    return QColor::fromRgbF(b.redF() * (1 - a) + c.redF() * a, b.greenF() * (1 - a) + c.greenF() * a, b.blueF() * (1 - a) + c.blueF() * a);
}

QColor AdwaitaStyle::buttonFill(ButtonKind k, const QStyleOption *opt) const
{
    const AdwaitaTokens &t = m_tokens;
    const bool down = opt->state & (State_Sunken | State_On);
    const bool hover = opt->state & State_MouseOver;
    QColor base, hov, act;
    switch (k) {
    case Suggested: base = t.accent_bg; hov = over(base, t.accent_fg, 0.10); act = over(base, QColor(0, 0, 6), 0.20); break;
    case Destructive:  // libadwaita 1.10: not filled; destructive-color text on a 15% / 20% / 35% tint
        base = withAlphaF(t.destructive_color, 0.15); hov = withAlphaF(t.destructive_color, 0.20); act = withAlphaF(t.destructive_color, 0.35); break;
    case Flat: base = Qt::transparent; hov = withAlphaF(t.window_fg, 0.07); act = withAlphaF(t.window_fg, 0.16); break;
    default: base = t.button_bg; hov = t.button_hover_bg; act = t.button_active_bg; break;
    }
    return withDisabled(down ? act : hover ? hov : base, opt);
}
