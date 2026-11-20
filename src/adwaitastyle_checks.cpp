// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyle.h"
#include <QPainter>
#include <QStyleOption>

bool AdwaitaStyle::drawCheckPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    if (pe != PE_IndicatorCheckBox && pe != PE_IndicatorRadioButton) return false;
    const AdwaitaTokens &t = m_tokens;
    const bool on = opt->state & State_On;
    const bool partial = opt->state & State_NoChange;
    const bool hover = opt->state & State_MouseOver;
    const QRectF r = QRectF(opt->rect);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);

    if (pe == PE_IndicatorCheckBox && isSwitch(w)) {
        const QColor bg = on ? t.accent_bg : (hover ? t.control_bg.lighter(110) : t.control_bg);
        p->setBrush(withDisabled(bg, opt));
        p->drawRoundedRect(r, r.height() / 2, r.height() / 2);
        const qreal k = r.height() - 6;  // knob
        const qreal x = on ? r.right() - 3 - k : r.left() + 3;
        QColor knob = (on ? t.accent_fg : QColor("#ffffff"));
        p->setBrush(withDisabled(knob, opt));
        p->drawEllipse(QRectF(x, r.top() + 3, k, k));
        p->restore();
        if (focusVisible(opt, w)) drawFocusRing(p, r, r.height() / 2);
        return true;
    }

    const bool radio = pe == PE_IndicatorRadioButton;
    const qreal rad = radio ? r.height() / 2 : t.radius_check;
    const bool filled = on || partial;
    QColor bg = filled ? t.accent_bg : t.control_bg;
    if (hover && !filled) bg = bg.lighter(110);
    if (opt->state & State_Sunken) bg = filled ? bg.darker(110) : t.button_active_bg;
    p->setBrush(withDisabled(bg, opt));
    p->drawRoundedRect(r, rad, rad);
    if (t.variant.startsWith("hc-") && !filled) {
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(t.control_border, 1));
        p->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), rad, rad);
    }
    if (filled) {
        const QColor fg = withDisabled(t.accent_fg, opt);
        const QPointF c = r.center();
        if (radio && !partial) {
            p->setPen(Qt::NoPen); p->setBrush(fg);
            p->drawEllipse(c, 3.5, 3.5);
        } else {
            p->setPen(QPen(fg, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p->setBrush(Qt::NoBrush);
            if (partial) p->drawLine(c + QPointF(-4, 0), c + QPointF(4, 0));
            else p->drawPolyline(QPolygonF({c + QPointF(-4.5, 0.2), c + QPointF(-1.5, 3.5), c + QPointF(4.5, -3.5)}));
        }
    }
    p->restore();
    if (focusVisible(opt, w)) drawFocusRing(p, r, rad);
    return true;
}
