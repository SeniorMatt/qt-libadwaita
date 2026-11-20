// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyle.h"
#include <QLineEdit>
#include <QPainter>
#include <QStyleOption>

bool AdwaitaStyle::drawEntryPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    if (pe == PE_FrameLineEdit) {
        return true;  // everything is drawn by PE_PanelLineEdit
    }
    if (pe != PE_PanelLineEdit) return false;
    // A QLineEdit embedded in a spin box / combo box is a bare field: the container draws the chrome.
    if (w && w->parentWidget() && (w->parentWidget()->inherits("QAbstractSpinBox") || w->parentWidget()->inherits("QComboBox")))
        return true;
    const QRectF r(opt->rect);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(withDisabled(t.entry_bg, opt));
    p->drawRoundedRect(r, t.radius, t.radius);
    if (t.variant.startsWith("hc-")) {
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(withDisabled(t.border, opt), 1));
        p->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), t.radius, t.radius);
    }
    p->restore();
    // Entries show the ring on any focus (libadwaita: text fields always get it), not only keyboard focus.
    drawFocus(p, r, t.radius, opt->state & State_HasFocus, w);
    return true;
}
