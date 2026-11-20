// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyle.h"
#include "adwaitasettings.h"
#include <QCheckBox>
#include <QEasingCurve>
#include <QPainter>
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
        if (!g_inFadedLayer) {  // 0 2px 4px rgba(0,0,6,.20)
            p->setBrush(QColor(0, 0, 6, 12));
            for (int i = 0; i < 4; ++i) {
                const qreal grow = 2.0 - i * 0.6;
                p->drawEllipse(kr.translated(0, 2).adjusted(-grow, -grow, grow, grow));
            }
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
