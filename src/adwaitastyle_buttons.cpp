// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyle.h"
#include <QPainter>
#include <QStyleOption>
#include <QStyleOptionComboBox>

bool AdwaitaStyle::drawButtonBevel(const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    const ButtonKind k = buttonKind(w, opt);
    const QRectF r(opt->rect);
    const qreal rad = k == Pill ? r.height() / 2 : t.radius;
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(buttonFill(k, opt));
    p->drawRoundedRect(r, rad, rad);
    if (t.variant.startsWith("hc-") && k != Flat) {  // high contrast: visible border
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(t.border, 1));
        p->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), rad, rad);
    }
    p->restore();
    if (focusVisible(opt, w)) drawFocusRing(p, r, rad);
    return true;
}

bool AdwaitaStyle::drawButtonLabel(const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const ButtonKind k = buttonKind(w, opt);
    QStyleOptionButton b = *qstyleoption_cast<const QStyleOptionButton *>(opt);
    if (k == Suggested) b.palette.setColor(QPalette::ButtonText, withDisabled(m_tokens.accent_fg, opt));
    else if (k == Destructive) b.palette.setColor(QPalette::ButtonText, withDisabled(m_tokens.destructive_fg, opt));
    QCommonStyle::drawControl(CE_PushButtonLabel, &b, p, w);
    return true;
}

bool AdwaitaStyle::drawComboBox(const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const
{
    auto *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
    if (!cb || !cb->frame) return false;
    const AdwaitaTokens &t = m_tokens;
    QStyleOption o = *opt;
    o.state &= ~State_On;
    if (cb->state & State_On) o.state |= State_Sunken;
    const QRectF r(opt->rect);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(buttonFill(Normal, &o));
    p->drawRoundedRect(r, t.radius, t.radius);
    // arrow: small chevron
    const QRect a = subControlRect(CC_ComboBox, opt, SC_ComboBoxArrow, w);
    QColor fg = withDisabled(t.window_fg, opt);
    p->setPen(QPen(fg, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const QPointF c = QRectF(a).center();
    p->drawPolyline(QPolygonF({c + QPointF(-4, -1.5), c + QPointF(0, 2.5), c + QPointF(4, -1.5)}));
    p->restore();
    if (focusVisible(opt, w)) drawFocusRing(p, r, t.radius);
    return true;
}
