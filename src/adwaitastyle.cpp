// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyle.h"
#include "adwaitasettings.h"

#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>
#include <QWidget>

AdwaitaStyle::AdwaitaStyle()
{
    reload();
    connect(AdwaitaSettings::instance(), &AdwaitaSettings::changed, this, &AdwaitaStyle::settingsChanged);
}

void AdwaitaStyle::reload()
{
    auto *s = AdwaitaSettings::instance();
    m_tokens = AdwaitaTokens::forVariant(s->dark(), s->highContrast(), s->accent());
}

void AdwaitaStyle::settingsChanged()
{
    reload();
    if (!qApp) return;
    QApplication::setPalette(standardPalette());
    // Re-polish every widget so cached state follows the new variant.
    const auto tops = QApplication::allWidgets();
    for (QWidget *w : tops) {
        QEvent e(QEvent::StyleChange);
        QApplication::sendEvent(w, &e);
        w->update();
    }
}

QPalette AdwaitaStyle::standardPalette() const
{
    const AdwaitaTokens &t = m_tokens;
    QPalette p;
    auto opaque = [&](const QColor &c, const QColor &over) {  // flatten translucent colour onto a background
        QColor r = c;
        if (c.alpha() == 255) return r;
        qreal a = c.alphaF();
        r.setRgbF(c.redF() * a + over.redF() * (1 - a), c.greenF() * a + over.greenF() * (1 - a),
                  c.blueF() * a + over.blueF() * (1 - a));
        return r;
    };
    const QColor fg = opaque(t.window_fg, t.window_bg), vfg = opaque(t.view_fg, t.view_bg);
    p.setColor(QPalette::Window, t.window_bg);
    p.setColor(QPalette::WindowText, fg);
    p.setColor(QPalette::Base, t.view_bg);
    p.setColor(QPalette::AlternateBase, opaque(t.shade, t.view_bg));
    p.setColor(QPalette::Text, vfg);
    p.setColor(QPalette::Button, opaque(t.button_bg, t.window_bg));
    p.setColor(QPalette::ButtonText, fg);
    p.setColor(QPalette::BrightText, Qt::white);
    p.setColor(QPalette::Highlight, t.accent_bg);
    p.setColor(QPalette::HighlightedText, t.accent_fg);
    p.setColor(QPalette::ToolTipBase, opaque(t.tooltip_bg, t.window_bg));
    p.setColor(QPalette::ToolTipText, t.tooltip_fg);
    p.setColor(QPalette::Link, t.accent_color);
    p.setColor(QPalette::LinkVisited, t.accent_color);
    p.setColor(QPalette::PlaceholderText, opaque(QColor(fg.red(), fg.green(), fg.blue(), 128), t.view_bg));
    p.setColor(QPalette::Light, t.window_bg.lighter(110));
    p.setColor(QPalette::Midlight, t.window_bg);
    p.setColor(QPalette::Mid, opaque(t.border, t.window_bg));
    p.setColor(QPalette::Dark, opaque(t.border, t.window_bg).darker(120));
    p.setColor(QPalette::Shadow, Qt::black);
    const qreal d = t.disabled_opacity;
    auto dis = [&](const QColor &c, const QColor &bg) { return opaque(QColor(c.red(), c.green(), c.blue(), int(255 * d)), bg); };
    p.setColor(QPalette::Disabled, QPalette::WindowText, dis(fg, t.window_bg));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, dis(fg, t.window_bg));
    p.setColor(QPalette::Disabled, QPalette::Text, dis(vfg, t.view_bg));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, dis(t.accent_fg, t.accent_bg));
    return p;
}

void AdwaitaStyle::polish(QPalette &pal)
{
    pal = standardPalette();
}

void AdwaitaStyle::polish(QWidget *w)
{
#define X(F) F##_polish(w);
    ADW_FAMILIES(X)
#undef X
    QCommonStyle::polish(w);
    polishSwitch(w);
    if (qobject_cast<QAbstractButton *>(w) || qobject_cast<QComboBox *>(w) || qobject_cast<QLineEdit *>(w))
        w->setAttribute(Qt::WA_Hover, true);
    const QString c = adwClass(w);
    if (c == "headerbar" || c == "boxed-list" || c == "boxed-row")
        w->setAttribute(Qt::WA_StyledBackground, true);
}

void AdwaitaStyle::unpolish(QWidget *w)
{
#define X(F) F##_unpolish(w);
    ADW_FAMILIES(X)
#undef X
    if (qobject_cast<QAbstractButton *>(w) || qobject_cast<QComboBox *>(w) || qobject_cast<QLineEdit *>(w))
        w->setAttribute(Qt::WA_Hover, false);
    QCommonStyle::unpolish(w);
}

// ---- helpers --------------------------------------------------------------------------------------------

QString AdwaitaStyle::adwClass(const QWidget *w)
{
    return w ? w->property("adwaita-class").toString() : QString();
}

bool AdwaitaStyle::isSwitch(const QWidget *w)
{
    return w && w->property("adwaita-switch").toBool();
}

AdwaitaStyle::ButtonKind AdwaitaStyle::buttonKind(const QWidget *w, const QStyleOption *opt)
{
    const QString c = adwClass(w);
    if (c == "suggested-action") return Suggested;
    if (c == "destructive-action") return Destructive;
    if (c == "pill") return Pill;
    if (c == "flat") return Flat;
    if (auto *b = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
        if (b->features & QStyleOptionButton::DefaultButton) return Suggested;
        if (b->features & QStyleOptionButton::Flat) return Flat;
    }
    return Normal;
}

bool AdwaitaStyle::focusVisible(const QStyleOption *opt, const QWidget *w)
{
    if (!(opt->state & QStyle::State_HasFocus)) return false;
    if (w && w->property("adwaita-focus-visible").toBool()) return true;
    return opt->state & QStyle::State_KeyboardFocusChange;
}

QColor AdwaitaStyle::withDisabled(const QColor &c, const QStyleOption *opt) const
{
    QColor r = c;
    if (!(opt->state & State_Enabled)) r.setAlphaF(c.alphaF() * m_tokens.disabled_opacity);
    return r;
}

void AdwaitaStyle::drawFocusRing(QPainter *p, const QRectF &r, qreal radius) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    const qreal w = m_tokens.focus_ring_width;
    QPen pen(m_tokens.focus_ring, w);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawRoundedRect(r.adjusted(w / 2, w / 2, -w / 2, -w / 2), radius - w / 2, radius - w / 2);
    p->restore();
}

// ---- dispatch -------------------------------------------------------------------------------------------

void AdwaitaStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
#define X(F) if (F##_drawPrimitive(pe, opt, p, w)) return;
    ADW_FAMILIES(X)
#undef X
    const AdwaitaTokens &t = m_tokens;
    switch (pe) {
    case PE_Widget: {
        const QString c = adwClass(w);
        if (c == "headerbar") {
            p->fillRect(opt->rect, t.headerbar_bg);
            p->fillRect(QRect(opt->rect.left(), opt->rect.bottom(), opt->rect.width(), 1), t.headerbar_border);
            return;
        }
        if (c == "boxed-list") {
            p->save(); p->setRenderHint(QPainter::Antialiasing);
            QRectF r = QRectF(opt->rect).adjusted(.5, .5, -.5, -.5);
            p->setBrush(t.card_bg); p->setPen(QPen(t.border, 1));
            p->drawRoundedRect(r, t.radius_card, t.radius_card);
            p->restore(); return;
        }
        if (c == "boxed-row") {  // separator on top of every row but the first
            if (w && w->property("adwaita-row-index").toInt() > 0)
                p->fillRect(QRect(opt->rect.left(), opt->rect.top(), opt->rect.width(), 1), t.border);
            return;
        }
        break;
    }
    case PE_FrameFocusRect:
        // Controls draw their own ring; the generic dotted rect is suppressed.
        return;
    case PE_PanelTipLabel: {
        p->save(); p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen); p->setBrush(t.tooltip_bg);
        p->drawRoundedRect(QRectF(opt->rect), t.radius, t.radius);
        p->restore(); return;
    }
    default: break;
    }
    if (drawEntryPrimitive(pe, opt, p, w)) return;
    if (drawCheckPrimitive(pe, opt, p, w)) return;
    QCommonStyle::drawPrimitive(pe, opt, p, w);
}

void AdwaitaStyle::drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
#define X(F) if (F##_drawControl(ce, opt, p, w)) return;
    ADW_FAMILIES(X)
#undef X
    switch (ce) {
    case CE_PushButtonBevel: if (drawButtonBevel(opt, p, w)) return; break;
    case CE_PushButtonLabel: if (drawButtonLabel(opt, p, w)) return; break;
    default: break;
    }
    QCommonStyle::drawControl(ce, opt, p, w);
}

void AdwaitaStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const
{
#define X(F) if (F##_drawComplexControl(cc, opt, p, w)) return;
    ADW_FAMILIES(X)
#undef X
    if (cc == CC_ComboBox && drawComboBox(opt, p, w)) return;
    QCommonStyle::drawComplexControl(cc, opt, p, w);
}

QRect AdwaitaStyle::subElementRect(SubElement se, const QStyleOption *opt, const QWidget *w) const
{
    { QRect out;
#define X(F) if (F##_subElementRect(se, opt, w, out)) return out;
    ADW_FAMILIES(X)
#undef X
    }
    const AdwaitaTokens &t = m_tokens;
    switch (se) {
    case SE_PushButtonContents:
    case SE_PushButtonFocusRect:
        return opt->rect;
    case SE_LineEditContents:
        return opt->rect.adjusted(int(t.entry_pad_x), 0, -int(t.entry_pad_x), 0);
    case SE_CheckBoxFocusRect:
    case SE_RadioButtonFocusRect:
        return opt->rect;
    default: break;
    }
    return QCommonStyle::subElementRect(se, opt, w);
}

QRect AdwaitaStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const
{
    { QRect out;
#define X(F) if (F##_subControlRect(cc, opt, sc, w, out)) return out;
    ADW_FAMILIES(X)
#undef X
    }
    if (cc == CC_ComboBox) {
        const QRect r = opt->rect;
        const int arrowW = 28;
        switch (sc) {
        case SC_ComboBoxFrame: return r;
        case SC_ComboBoxArrow: return QRect(r.right() - arrowW + 1, r.top(), arrowW, r.height());
        case SC_ComboBoxEditField: return QRect(r.left() + int(m_tokens.button_pad_x), r.top() + 1,
                                                r.width() - arrowW - int(m_tokens.button_pad_x), r.height() - 2);
        case SC_ComboBoxListBoxPopup: return r;
        default: break;
        }
    }
    return QCommonStyle::subControlRect(cc, opt, sc, w);
}

int AdwaitaStyle::pixelMetric(PixelMetric pm, const QStyleOption *opt, const QWidget *w) const
{
    { int out = 0;
#define X(F) if (F##_pixelMetric(pm, opt, w, out)) return out;
    ADW_FAMILIES(X)
#undef X
    }
    const AdwaitaTokens &t = m_tokens;
    switch (pm) {
    case PM_DefaultFrameWidth: return qobject_cast<const QLineEdit *>(w) ? 0 : QCommonStyle::pixelMetric(pm, opt, w);
    case PM_ButtonDefaultIndicator:
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical: return 0;
    case PM_IndicatorWidth: return isSwitch(w) ? int(t.switch_width) : int(t.check_size);
    case PM_IndicatorHeight: return isSwitch(w) ? int(t.switch_height) : int(t.check_size);
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight: return int(t.check_size);
    case PM_CheckBoxLabelSpacing:
    case PM_RadioButtonLabelSpacing: return 6;
    default: break;
    }
    return QCommonStyle::pixelMetric(pm, opt, w);
}

QSize AdwaitaStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &sz, const QWidget *w) const
{
    { QSize out;
#define X(F) if (F##_sizeFromContents(ct, opt, sz, w, out)) return out;
    ADW_FAMILIES(X)
#undef X
    }
    const AdwaitaTokens &t = m_tokens;
    switch (ct) {
    case CT_PushButton: {
        const bool pill = buttonKind(w, opt) == Pill;
        const int padX = pill ? 16 : int(t.button_pad_x);
        int extra = 0;  // the label is drawn bold
        if (auto *b = qstyleoption_cast<const QStyleOptionButton *>(opt)) if (!b->text.isEmpty()) {
            QFont bf = w ? w->font() : QApplication::font(); bf.setBold(true);
            extra = QFontMetrics(bf).horizontalAdvance(b->text) - b->fontMetrics.horizontalAdvance(b->text);
        }
        return QSize(qMax(sz.width() + extra + 2 * padX, int(t.button_min_height)), qMax(sz.height() + 10, int(t.button_min_height)));
    }
    case CT_LineEdit:
        return QSize(sz.width() + 2 * int(t.entry_pad_x), qMax(sz.height(), int(t.entry_min_height)));
    case CT_ComboBox:
        return QSize(sz.width() + int(t.button_pad_x) + 28, qMax(sz.height() + 10, int(t.button_min_height)));
    default: break;
    }
    return QCommonStyle::sizeFromContents(ct, opt, sz, w);
}

int AdwaitaStyle::styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *w, QStyleHintReturn *ret) const
{
    { int out = 0;
#define X(F) if (F##_styleHint(sh, opt, w, ret, out)) return out;
    ADW_FAMILIES(X)
#undef X
    }
    switch (sh) {
    case SH_Button_FocusPolicy: return Qt::StrongFocus;
    case SH_EtchDisabledText: return 0;
    case SH_Widget_Animate: return AdwaitaSettings::instance()->animationsEnabled();
    case SH_ComboBox_Popup: return 0;
    default: break;
    }
    return QCommonStyle::styleHint(sh, opt, w, ret);
}
