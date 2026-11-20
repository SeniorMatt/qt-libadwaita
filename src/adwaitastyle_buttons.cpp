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
    const qreal rad = isPillButton(w) ? r.height() / 2 : t.radius;
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(buttonFill(k, opt));
    p->drawRoundedRect(r, rad, rad);
    // high contrast: inset 1px border in currentColor at 50%; suggested has none (same colour as its fill),
    // flat only while hovered/pressed; destructive uses its red text colour.
    const bool flatActive = k == Flat && (opt->state & (State_MouseOver | State_Sunken | State_On));
    if (t.variant.startsWith("hc-") && k != Suggested && (k != Flat || flatActive)) {
        QColor bc = k == Destructive ? t.destructive_color : t.window_fg;
        bc.setAlphaF(bc.alphaF() * t.border_opacity);
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(withDisabled(bc, opt), 1));
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
    else if (k == Destructive) b.palette.setColor(QPalette::ButtonText, withDisabled(m_tokens.destructive_color, opt));
    p->save();
    QFont bf = p->font(); bf.setBold(true); bf.setStyleStrategy(QFont::NoSubpixelAntialias); p->setFont(bf);  // libadwaita button label weight 700
    b.fontMetrics = QFontMetrics(bf);
    QCommonStyle::drawControl(CE_PushButtonLabel, &b, p, w);
    p->restore();
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
    if (t.variant.startsWith("hc-")) {
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(withDisabled(t.border, opt), 1));
        p->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), t.radius - .5, t.radius - .5);
        p->setPen(Qt::NoPen);
    }
    // arrow: small chevron
    const QRect a = subControlRect(CC_ComboBox, opt, SC_ComboBoxArrow, w);
    QColor fg = withDisabled(t.window_fg, opt);
        const QPointF c = QRectF(a).center() - QPointF(5, 0);  // 16px icon box at the left of the 26px arrow area
    p->setPen(QPen(fg, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPolyline(QPolygonF({c + QPointF(-4.5, -1.5), c + QPointF(0, 3), c + QPointF(4.5, -1.5)}));
    p->restore();
    if (focusVisible(opt, w)) drawFocusRing(p, r, t.radius);
    return true;
}

bool AdwaitaStyle::drawComboLabel(const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    auto *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
    if (!cb || cb->editable) return false;
    QRect r = subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, w);
    p->save();
    QFont bf = p->font(); bf.setBold(true); bf.setStyleStrategy(QFont::NoSubpixelAntialias); p->setFont(bf);
    if (!cb->currentIcon.isNull()) {
        const QIcon::Mode im = (cb->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QPixmap pm = cb->currentIcon.pixmap(cb->iconSize, p->device()->devicePixelRatio(), im);
#else
        const qreal dpr = p->device()->devicePixelRatioF();
        QPixmap pm = cb->currentIcon.pixmap(cb->iconSize * dpr, im);
        pm.setDevicePixelRatio(dpr);
#endif
        p->drawPixmap(QRect(r.left(), r.top() + (r.height() - cb->iconSize.height()) / 2, cb->iconSize.width(), cb->iconSize.height()), pm);
        r.setLeft(r.left() + cb->iconSize.width() + 6);
    }
    p->setPen(withDisabled(m_tokens.window_fg, opt));
    p->drawText(r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, p->fontMetrics().elidedText(cb->currentText, Qt::ElideRight, r.width()));
    p->restore();
    return true;
}
