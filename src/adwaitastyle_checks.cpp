// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyle.h"
#include "adwaitasettings.h"
#include <QCheckBox>
#include <QEasingCurve>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>
#include <QVariantAnimation>

namespace {
bool g_inFadedLayer = false;  // knob shadow is dropped on disabled switches (libadwaita)
const char kAnimName[] = "adwSwitchAnim";

QColor mixc(const QColor &a, const QColor &b, qreal t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t, a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}
}  // namespace

// Knob slide animation: one QVariantAnimation child per switch (200 ms, ease-out-quad = the button transition token).
void AdwaitaStyle::polishSwitch(QWidget *w)
{
    auto *cb = qobject_cast<QCheckBox *>(w);
    if (!cb || cb->findChild<QVariantAnimation *>(kAnimName)) return;
    auto *anim = new QVariantAnimation(cb);
    anim->setObjectName(kAnimName);
    anim->setDuration(200);
    QEasingCurve ec(QEasingCurve::BezierSpline);
    ec.addCubicBezierSegment(QPointF(0.25, 0.46), QPointF(0.45, 0.94), QPointF(1, 1));
    anim->setEasingCurve(ec);
    connect(anim, &QVariantAnimation::valueChanged, cb, [cb] { cb->update(); });
    connect(anim, &QVariantAnimation::finished, cb, [cb] { cb->update(); });
    connect(cb, &QAbstractButton::toggled, anim, [cb, anim](bool on) {
        if (!isSwitch(cb)) return;
        const qreal from = anim->state() == QAbstractAnimation::Running ? anim->currentValue().toReal() : (on ? 0.0 : 1.0);
        anim->stop();
        if (!AdwaitaSettings::instance()->animationsEnabled() || !cb->isVisible()) { cb->update(); return; }
        anim->setStartValue(from);
        anim->setEndValue(on ? 1.0 : 0.0);
        anim->start();
    });
}

bool AdwaitaStyle::drawCheckPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    if (pe != PE_IndicatorCheckBox && pe != PE_IndicatorRadioButton) return false;
    if (!(opt->state & State_Enabled)) {
        QStyleOption en = *opt;
        en.state |= State_Enabled;
        en.state &= ~(State_MouseOver | State_Sunken);
        const bool prev = g_inFadedLayer;
        g_inFadedLayer = true;
        drawFaded(p, opt->rect, [&](QPainter *ip) { drawCheckPrimitive(pe, &en, ip, w); });
        g_inFadedLayer = prev;
        return true;
    }
    const AdwaitaTokens &t = m_tokens;
    const bool on = opt->state & State_On;
    const bool partial = opt->state & State_NoChange;
    const bool hover = opt->state & State_MouseOver;
    const QRectF r = QRectF(opt->rect);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);

    if (pe == PE_IndicatorCheckBox && isSwitch(w)) {
        qreal prog = on ? 1.0 : 0.0;
        if (w)
            if (auto *anim = w->findChild<QVariantAnimation *>(kAnimName))
                if (anim->state() == QAbstractAnimation::Running) prog = anim->currentValue().toReal();
        QColor off = hover ? t.control_bg.lighter(110) : t.control_bg;
        p->setBrush(mixc(off, t.accent_bg, prog));
        p->drawRoundedRect(r, r.height() / 2, r.height() / 2);
        const qreal k = r.height() - 6;  // knob
        const qreal x = r.left() + 3 + prog * (r.width() - 6 - k);
        const QRectF kr(x, r.top() + 3, k, k);
        if (!g_inFadedLayer) {  // 0 2px 4px rgba(0,0,6,.20), clipped to the trough
            QPainterPath clip;
            clip.addRoundedRect(r, r.height() / 2, r.height() / 2);
            p->save();
            if (t.variant.endsWith("dark")) p->setClipPath(clip);  // libadwaita: shadow only leaks out on light schemes
            p->setBrush(QColor(0, 0, 6, 8));
            for (qreal grow : {3.0, 2.2, 1.4, 0.6, -0.2, -1.0})
                p->drawEllipse(kr.translated(0, 2).adjusted(-grow, -grow, grow, grow));
            p->restore();
        }
        const QColor white(255, 255, 255);
        const QColor viewBg = t.view_bg;
        const QColor idle = QColor::fromRgbF(white.redF() * .8 + viewBg.redF() * .2, white.greenF() * .8 + viewBg.greenF() * .2,
                                             white.blueF() * .8 + viewBg.blueF() * .2);
        p->setBrush((hover || on || prog > 0) ? white : idle);
        p->drawEllipse(kr);
        p->restore();
        if (focusVisible(opt, w)) drawFocusRing(p, r, r.height() / 2);
        return true;
    }

    const bool radio = pe == PE_IndicatorRadioButton;
    const qreal rad = radio ? r.height() / 2 : t.radius_check;
    const bool filled = on || partial;
    const bool hc = t.variant.startsWith("hc-");
    const bool sunk = opt->state & State_Sunken;
    auto cc = [&](qreal a) { QColor c = t.window_fg; c.setAlphaF(c.alphaF() * a); return c; };
    if (filled) {
        p->setBrush(t.accent_bg);
        p->drawRoundedRect(r, rad, rad);
        if (sunk) { p->setBrush(QColor(0, 0, 6, 51)); p->drawRoundedRect(r, rad, rad); }
        else if (hover) { p->setBrush(cc(0.10)); p->drawRoundedRect(r, rad, rad); }
    } else if (sunk) {  // active: fill cc 25% (hc 60%), no ring
        p->setBrush(cc(hc ? 0.60 : 0.25));
        p->drawRoundedRect(r, rad, rad);
    } else {  // inset 2px ring cc 15% (hover 20; hc 50 / 60)
        QPainterPath ring;
        ring.addRoundedRect(r, rad, rad);
        ring.addRoundedRect(r.adjusted(2, 2, -2, -2), qMax<qreal>(0, rad - 2), qMax<qreal>(0, rad - 2));
        ring.setFillRule(Qt::OddEvenFill);
        p->setBrush(cc(hc ? (hover ? 0.60 : 0.50) : (hover ? 0.20 : 0.15)));
        p->drawPath(ring);
    }
    if (filled) {
        const QColor fg = withDisabled(t.accent_fg, opt);
        const QPointF c = r.center();
        if (radio && !partial) {
            p->setPen(Qt::NoPen); p->setBrush(fg);
            p->drawEllipse(c, 4.0, 4.0);
        } else {
            p->setPen(QPen(fg, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p->setBrush(Qt::NoBrush);
            if (partial) p->drawLine(c + QPointF(-4, 0), c + QPointF(4, 0));
            else p->drawPolyline(QPolygonF({c + QPointF(-5.0, 0.1), c + QPointF(-1.2, 3.8), c + QPointF(5.2, -4.0)}));
        }
    }
    p->restore();
    if (focusVisible(opt, w)) drawFocusRing(p, r, rad);
    return true;
}
