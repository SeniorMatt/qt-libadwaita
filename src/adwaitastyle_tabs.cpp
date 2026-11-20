// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: tab bars, tab widgets, tool bars, tool buttons, group boxes, frames, splitters
#include "adwaitastyle.h"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QDockWidget>
#include <QFontMetrics>
#include <QPainter>
#include <QStyleOptionDockWidget>
#include <QStyleOptionFrame>
#include <QStyleOptionGroupBox>
#include <QStyleOptionTab>
#include <QStyleOptionTabBarBase>
#include <QStyleOptionTabWidgetFrame>
#include <QStyleOptionToolButton>
#include <QToolBar>
#include <QToolButton>

namespace {

constexpr int kTabH = 38;       // tab bar height; the tab shape is inset 6px (26px tall, radius 9)
constexpr int kTabInset = 6;
constexpr int kMenuArrow = 16;

QColor alpha(QColor c, qreal a) { c.setAlphaF(a); return c; }

bool horizontal(QTabBar::Shape s) { return s == QTabBar::RoundedNorth || s == QTabBar::RoundedSouth || s == QTabBar::TriangularNorth || s == QTabBar::TriangularSouth; }

QFont boldFont(const QWidget *w)
{
    QFont f = w ? w->font() : QApplication::font();
    f.setBold(true);
    return f;
}

}  // namespace

bool AdwaitaStyle::tabs_drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (pe) {
    case PE_FrameTabBarBase: {
        auto *b = qstyleoption_cast<const QStyleOptionTabBarBase *>(opt);
        if (!b) return false;
        // bottom (or top) line like the tab bar's headerbar-shade
        const QRect r = opt->rect;
        const bool south = b->shape == QTabBar::RoundedSouth || b->shape == QTabBar::TriangularSouth;
        const QColor c = alpha(t.headerbar_fg, t.border_opacity);
        if (horizontal(b->shape)) {
            // only in a plain QTabBar; inside a QTabWidget the frame draws the line
            if (qobject_cast<const QTabBar *>(w) && !(w->parentWidget() && w->parentWidget()->inherits("QTabWidget")))
                p->fillRect(QRect(r.left(), south ? r.top() : r.bottom(), r.width(), 1), c);
        }
        return true;
    }
    case PE_FrameTabWidget: {
        auto *f = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt);
        p->save();
        p->fillRect(opt->rect, t.view_bg);
        p->setPen(QPen(t.border, 1));
        p->setBrush(Qt::NoBrush);
        p->drawRect(QRectF(opt->rect).adjusted(.5, .5, -.5, -.5));
        p->restore();
        Q_UNUSED(f);
        return true;
    }
    case PE_IndicatorTabClose: {
        const bool hover = opt->state & State_MouseOver, down = opt->state & State_Sunken;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QRectF r = QRectF(opt->rect);
        if (hover || down) {
            p->setPen(Qt::NoPen);
            p->setBrush(alpha(t.window_fg, down ? 0.16 : 0.10));
            const qreal s = qMin(r.width(), r.height());
            p->drawEllipse(r.center(), s / 2, s / 2);
        }
        QColor c = t.window_fg;
        if (!(hover || down)) c.setAlphaF(c.alphaF() * 0.55);
        p->setPen(QPen(c, 1.6, Qt::SolidLine, Qt::RoundCap));
        const QPointF m = r.center();
        p->drawLine(m + QPointF(-3.5, -3.5), m + QPointF(3.5, 3.5));
        p->drawLine(m + QPointF(-3.5, 3.5), m + QPointF(3.5, -3.5));
        p->restore();
        return true;
    }
    case PE_PanelToolBar:
        return true;  // drawn by CE_ToolBar
    case PE_IndicatorToolBarSeparator: {
        const QRect r = opt->rect;
        if (opt->state & State_Horizontal) p->fillRect(QRect(r.center().x(), r.top() + 6, 1, r.height() - 12), t.border);
        else p->fillRect(QRect(r.left() + 6, r.center().y(), r.width() - 12, 1), t.border);
        return true;
    }
    case PE_IndicatorToolBarHandle:
        return true;
    case PE_FrameGroupBox:
        return true;  // no frame: bold title only
    case PE_Frame:
    case PE_FrameDockWidget:
        return false;
    default: return false;
    }
}

bool AdwaitaStyle::tabs_drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (ce) {
    case CE_TabBarTabShape: {
        auto *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
        if (!tab) return false;
        const bool sel = tab->state & State_Selected;
        const bool hover = (tab->state & State_MouseOver) && (tab->state & State_Enabled);
        const bool down = tab->state & State_Sunken;
        QRectF r(opt->rect);
        if (horizontal(tab->shape)) r.adjust(2, kTabInset, -2, -kTabInset);
        else r.adjust(kTabInset, 2, -kTabInset, -2);
        qreal a = sel ? (hover ? 0.13 : 0.10) : hover ? 0.07 : 0;
        if (down) a = sel ? 0.19 : 0.16;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (a > 0) {
            p->setPen(Qt::NoPen);
            p->setBrush(alpha(t.window_fg, a));
            p->drawRoundedRect(r, t.radius, t.radius);
        }
        p->restore();
        if (focusVisible(opt, w) && (tab->state & State_Selected)) drawFocusRing(p, r, t.radius);
        return true;
    }
    case CE_TabBarTabLabel: {
        auto *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
        if (!tab) return false;
        QStyleOptionTab o = *tab;
        o.palette.setColor(QPalette::WindowText, withDisabled(t.window_fg, opt));
        o.palette.setColor(QPalette::ButtonText, withDisabled(t.window_fg, opt));
        o.state &= ~State_HasFocus;  // never the dotted focus rect
        QCommonStyle::drawControl(CE_TabBarTabLabel, &o, p, w);
        return true;
    }
    case CE_ToolBar: {
        auto *tb = qstyleoption_cast<const QStyleOptionToolBar *>(opt);
        if (!tb) return false;
        if (tb->state & State_Horizontal) {
            p->fillRect(opt->rect, t.headerbar_bg);
            p->fillRect(QRect(opt->rect.left(), opt->rect.bottom(), opt->rect.width(), 1), alpha(t.headerbar_fg, t.border_opacity));
        }
        return true;
    }
    case CE_DockWidgetTitle: {
        auto *d = qstyleoption_cast<const QStyleOptionDockWidget *>(opt);
        if (!d) return false;
        p->save();
        p->fillRect(opt->rect, t.window_bg);
        p->fillRect(QRect(opt->rect.left(), opt->rect.bottom(), opt->rect.width(), 1), t.border);
        p->setFont(boldFont(w));
        p->setPen(withDisabled(t.window_fg, opt));
        const QRect tr = subElementRect(SE_DockWidgetTitleBarText, opt, w).adjusted(6, 0, 0, 0);
        p->drawText(tr, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, p->fontMetrics().elidedText(d->title, Qt::ElideRight, tr.width()));
        p->restore();
        return true;
    }
    case CE_Splitter: {
        const bool h = opt->state & State_Horizontal;  // horizontal: handle is a vertical line
        const QRect r = opt->rect;
        p->fillRect(h ? QRect(r.center().x(), r.top(), 1, r.height()) : QRect(r.left(), r.center().y(), r.width(), 1), t.border);
        return true;
    }
    case CE_ShapedFrame: {
        auto *f = qstyleoption_cast<const QStyleOptionFrame *>(opt);
        if (!f) return false;
        const int shape = f->frameShape;
        if (shape == QFrame::HLine || shape == QFrame::VLine) {
            const QRect r = opt->rect;
            p->fillRect(shape == QFrame::HLine ? QRect(r.left(), r.center().y(), r.width(), 1) : QRect(r.center().x(), r.top(), 1, r.height()), t.border);
            return true;
        }
        if (shape == QFrame::Box || shape == QFrame::Panel || shape == QFrame::StyledPanel) {
            const bool scroll = qobject_cast<const QAbstractScrollArea *>(w);
            const qreal rad = scroll ? 0 : t.radius_card;
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setBrush(Qt::NoBrush);
            p->setPen(QPen(t.border, 1));
            p->drawRoundedRect(QRectF(opt->rect).adjusted(.5, .5, -.5, -.5), rad, rad);
            p->restore();
            return true;
        }
        return false;
    }
    default: return false;
    }
}

bool AdwaitaStyle::tabs_drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    if (cc == CC_ToolButton) {
        auto *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
        if (!tb) return false;
        const QString c = adwClass(w);
        const ButtonKind k = c.isEmpty() ? Flat : buttonKind(w, opt);
        const bool split = tb->subControls & SC_ToolButtonMenu;
        const bool arrow = (tb->features & QStyleOptionToolButton::HasMenu) && !split;
        const QRectF r(opt->rect);
        const qreal rad = k == Pill ? r.height() / 2 : t.radius;
        QStyleOption bo = *opt;
        if (tb->state & State_Raised) bo.state &= ~State_Sunken;
        if (!(tb->state & State_Enabled)) bo.state &= ~State_MouseOver;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        QStyleOption mainOpt = bo;
        if (split) {  // only the pressed part reacts
            const QRect mr = subControlRect(CC_ToolButton, opt, SC_ToolButtonMenu, w);
            mainOpt.state &= ~State_Sunken;
            if (tb->activeSubControls & SC_ToolButton) mainOpt.state |= (tb->state & State_Sunken);
            p->setBrush(buttonFill(k, &mainOpt));
            p->drawRoundedRect(r, rad, rad);
            QStyleOption menuOpt = bo;
            if (tb->activeSubControls & SC_ToolButtonMenu) menuOpt.state |= State_Sunken; else menuOpt.state &= ~State_Sunken;
            p->setBrush(buttonFill(k, &menuOpt));
            p->drawRoundedRect(QRectF(mr).adjusted(0, 0, 0, 0), rad, rad);
            p->fillRect(QRect(mr.left(), mr.top() + 6, 1, mr.height() - 12), t.border);
        } else {
            p->setBrush(buttonFill(k, &bo));
            p->drawRoundedRect(r, rad, rad);
        }
        if (t.variant.startsWith("hc-") && k != Flat) {
            p->setBrush(Qt::NoBrush); p->setPen(QPen(t.border, 1));
            p->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), rad, rad);
        }
        p->restore();
        QStyleOptionToolButton lo = *tb;
        lo.rect = split ? subControlRect(CC_ToolButton, opt, SC_ToolButton, w) : opt->rect;
        if (arrow) lo.rect.adjust(0, 0, -kMenuArrow, 0);
        lo.features &= ~QStyleOptionToolButton::HasMenu;
        lo.state &= ~(State_Sunken | State_HasFocus);
        if (k == Suggested) lo.palette.setColor(QPalette::ButtonText, withDisabled(t.accent_fg, opt));
        else if (k == Destructive) lo.palette.setColor(QPalette::ButtonText, withDisabled(t.destructive_fg, opt));
        else lo.palette.setColor(QPalette::ButtonText, withDisabled(t.window_fg, opt));
        lo.palette.setColor(QPalette::WindowText, lo.palette.color(QPalette::ButtonText));
        QCommonStyle::drawControl(CE_ToolButtonLabel, &lo, p, w);
        if (arrow || split) {
            const QRect ar = split ? subControlRect(CC_ToolButton, opt, SC_ToolButtonMenu, w)
                                   : QRect(opt->rect.right() - kMenuArrow - 1, opt->rect.top(), kMenuArrow, opt->rect.height());
            QColor ac = k == Suggested ? t.accent_fg : k == Destructive ? t.destructive_fg : t.window_fg;
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setPen(QPen(withDisabled(ac, opt), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            const QPointF c2 = QRectF(ar).center();
            p->drawPolyline(QPolygonF({c2 + QPointF(-3.5, -1.5), c2 + QPointF(0, 2), c2 + QPointF(3.5, -1.5)}));
            p->restore();
        }
        if (focusVisible(opt, w)) drawFocusRing(p, r, rad);
        return true;
    }
    if (cc == CC_GroupBox) {
        auto *g = qstyleoption_cast<const QStyleOptionGroupBox *>(opt);
        if (!g) return false;
        p->save();
        if (g->subControls & SC_GroupBoxCheckBox) {
            QStyleOptionButton cb;
            cb.QStyleOption::operator=(*opt);
            cb.rect = subControlRect(CC_GroupBox, opt, SC_GroupBoxCheckBox, w);
            cb.state = (opt->state & ~(State_Sunken | State_MouseOver)) | ((g->state & State_On) ? State_On : State_Off);
            if (g->activeSubControls & SC_GroupBoxCheckBox) cb.state |= State_MouseOver;
            drawPrimitive(PE_IndicatorCheckBox, &cb, p, w);
        }
        if (g->subControls & SC_GroupBoxLabel) {
            QFont f = boldFont(w);
            p->setFont(f);
            p->setPen(withDisabled(t.window_fg, opt));
            const QRect tr = subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, w);
            int flags = Qt::AlignVCenter | Qt::AlignLeft | Qt::TextShowMnemonic | Qt::TextSingleLine;
            if (!proxy()->styleHint(SH_UnderlineShortcut, opt, w)) flags |= Qt::TextHideMnemonic;
            p->drawText(tr, flags, g->text);
        }
        p->restore();
        return true;
    }
    return false;
}

bool AdwaitaStyle::tabs_subElementRect(SubElement se, const QStyleOption *opt, const QWidget *w, QRect &out) const
{
    Q_UNUSED(w);
    switch (se) {
    case SE_TabBarTabLeftButton:
    case SE_TabBarTabRightButton:
    case SE_TabBarTabText:
        return false;
    case SE_ToolBarHandle:
        out = QRect();
        return false;
    default: break;
    }
    Q_UNUSED(opt);
    return false;
}

bool AdwaitaStyle::tabs_subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *w, QRect &out) const
{
    if (cc == CC_ToolButton) {
        auto *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
        if (!tb) return false;
        const QRect r = opt->rect;
        const bool split = tb->subControls & SC_ToolButtonMenu;
        switch (sc) {
        case SC_ToolButton: out = split ? r.adjusted(0, 0, -kMenuArrow - 4, 0) : r; return true;
        case SC_ToolButtonMenu: out = split ? QRect(r.right() - kMenuArrow - 3, r.top(), kMenuArrow + 4, r.height()) : QRect(); return true;
        default: return false;
        }
    }
    if (cc == CC_GroupBox) {
        auto *g = qstyleoption_cast<const QStyleOptionGroupBox *>(opt);
        if (!g) return false;
        const QFontMetrics fm(boldFont(w));
        const int th = qMax(fm.height(), 20);
        const int cbw = (g->subControls & SC_GroupBoxCheckBox) ? int(m_tokens.check_size) + 6 : 0;
        const int tw = fm.horizontalAdvance(g->text, -1) + 2;
        switch (sc) {
        case SC_GroupBoxCheckBox: out = QRect(0, (th - int(m_tokens.check_size)) / 2, int(m_tokens.check_size), int(m_tokens.check_size)); return true;
        case SC_GroupBoxLabel: out = QRect(cbw, 0, qMin(tw, opt->rect.width() - cbw), th); return true;
        case SC_GroupBoxFrame: out = opt->rect; return true;
        case SC_GroupBoxContents: out = opt->rect.adjusted(0, th + 6, 0, 0); return true;
        default: return false;
        }
    }
    return false;
}

bool AdwaitaStyle::tabs_pixelMetric(PixelMetric pm, const QStyleOption *, const QWidget *, int &out) const
{
    switch (pm) {
    case PM_TabBarTabHSpace: out = 28; return true;
    case PM_TabBarTabVSpace: out = 0; return true;
    case PM_TabBarTabShiftHorizontal:
    case PM_TabBarTabShiftVertical:
    case PM_TabBarBaseOverlap:
    case PM_TabBarBaseHeight:
    case PM_TabBarTabOverlap: out = 0; return true;
    case PM_TabBarScrollButtonWidth: out = 24; return true;
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight: out = 20; return true;
    case PM_ToolBarFrameWidth: out = 0; return true;
    case PM_ToolBarItemMargin: out = 6; return true;
    case PM_ToolBarItemSpacing: out = 6; return true;
    case PM_ToolBarHandleExtent: out = 8; return true;
    case PM_ToolBarSeparatorExtent: out = 13; return true;
    case PM_ToolBarExtensionExtent: out = 24; return true;
    case PM_ToolBarIconSize: out = 16; return true;
    case PM_MenuButtonIndicator: out = kMenuArrow; return true;
    case PM_SplitterWidth: out = 5; return true;
    case PM_DockWidgetTitleMargin: out = 6; return true;
    case PM_DockWidgetTitleBarButtonMargin: out = 4; return true;
    case PM_DockWidgetFrameWidth: out = 1; return true;
    case PM_DockWidgetSeparatorExtent: out = 5; return true;
    default: return false;
    }
}

bool AdwaitaStyle::tabs_sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &sz, const QWidget *w, QSize &out) const
{
    switch (ct) {
    case CT_TabBarTab: {
        auto *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
        QSize s = QCommonStyle::sizeFromContents(ct, opt, sz, w);
        if (tab && horizontal(tab->shape)) s.setHeight(qMax(s.height(), kTabH));
        else s.setWidth(qMax(s.width(), kTabH));
        out = s;
        return true;
    }
    case CT_ToolButton: {
        auto *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
        const bool hasText = tb && !tb->text.isEmpty() && tb->toolButtonStyle != Qt::ToolButtonIconOnly;
        int wd = sz.width() + (hasText ? 18 : 10);
        out = QSize(qMax(wd, 34), qMax(sz.height() + 10, 34));
        return true;
    }
    case CT_Splitter:
        return false;
    default: return false;
    }
}

bool AdwaitaStyle::tabs_styleHint(StyleHint sh, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &out) const
{
    switch (sh) {
    case SH_TabBar_Alignment: out = Qt::AlignLeft; return true;
    case SH_TabBar_ElideMode: out = Qt::ElideRight; return true;
    case SH_TabBar_CloseButtonPosition: out = QTabBar::RightSide; return true;
    case SH_TabBar_PreferNoArrows: out = 1; return true;
    case SH_ToolButtonStyle: out = Qt::ToolButtonIconOnly; return false;
    case SH_GroupBox_TextLabelColor: return false;
    case SH_DockWidget_ButtonsHaveFrame: out = 0; return true;
    default: return false;
    }
}

void AdwaitaStyle::tabs_polish(QWidget *w)
{
    if (qobject_cast<QToolButton *>(w) || qobject_cast<QTabBar *>(w)) w->setAttribute(Qt::WA_Hover, true);
    if (auto *tb = qobject_cast<QToolBar *>(w)) tb->setAttribute(Qt::WA_StyledBackground, true);
}

void AdwaitaStyle::tabs_unpolish(QWidget *) {}
