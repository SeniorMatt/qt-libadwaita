// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: tab bars, tab widgets, tool bars, tool buttons, group boxes, frames, splitters
#include "adwaitastyle.h"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QDockWidget>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
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

constexpr int kTabH = 47;       // AdwTabBar: 6 + 34 (26 min + 2*4 padding) + 6, plus the 1px bottom line; the tab shape is 34px tall, radius 9
constexpr int kTabInset = 6;
constexpr int kMenuArrow = 16;

QColor alpha(QColor c, qreal a) { c.setAlphaF(a); return c; }

// Colours measured from the libadwaita 1.10 reference shots (tests/visual/reference/<scheme>/blue/{tabs,headerbar,boxedlist}@1x.png).
bool isDark(const AdwaitaTokens &t) { return t.variant.endsWith("dark"); }
bool isHc(const AdwaitaTokens &t) { return t.variant.startsWith("hc-"); }
QColor barBg(const AdwaitaTokens &t) { return isDark(t) ? QColor(0x2e, 0x2e, 0x32) : QColor(0xff, 0xff, 0xff); }
QColor barLine(const AdwaitaTokens &t)
{
    if (t.variant == "hc-light") return QColor(0x99, 0x99, 0x9b);
    if (t.variant == "hc-dark") return QColor(0x97, 0x97, 0x99);
    return isDark(t) ? QColor(0x1d, 0x1d, 0x22) : QColor(0xe1, 0xe1, 0xe1);
}
QColor tabFill(const AdwaitaTokens &t, qreal level)  // level 0.8 = selected, 0.5 = hover, 1.6 = pressed (x of the base .10)
{
    return alpha(isDark(t) ? QColor(Qt::white) : QColor(Qt::black), (isDark(t) ? 0.10 : 0.08) * level / 0.8);
}
QColor listSep(const AdwaitaTokens &t)
{
    if (isHc(t)) return QColor(0x99, 0x99, 0x9b);
    return isDark(t) ? QColor(0x21, 0x21, 0x25) : QColor(0xee, 0xee, 0xee);
}
QColor listRing(const AdwaitaTokens &t)
{
    if (t.variant == "hc-light") return QColor(0x90, 0x90, 0x93);
    if (t.variant == "hc-dark") return QColor(0x8c, 0x8c, 0x8e);
    return isDark(t) ? QColor(0x20, 0x20, 0x24) : QColor(0xe4, 0xe4, 0xe5);
}

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
        const QColor c = barLine(t);
        if (horizontal(b->shape)) {
            // only in a plain QTabBar; inside a QTabWidget the frame draws the line
            if (qobject_cast<const QTabBar *>(w) && !(w->parentWidget() && w->parentWidget()->inherits("QTabWidget"))) {
                p->fillRect(r, barBg(t));  // AdwTabBar background
                p->fillRect(QRect(r.left(), south ? r.top() : r.bottom(), r.width(), 1), c);
            }
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
    case PE_Widget: {
        const QString c = adwClass(w);
        if (c == "headerbar") {
            // libadwaita's header bar is square; the 15 px window corners (window_radius) clip it when it is the top
            // of a window. Qt only needs that for a frameless window drawing its own title bar (client-side
            // decoration); under a window manager title bar, or embedded in content, it stays square.
            const QWidget *win = w ? w->window() : nullptr;
            const bool csdTop = win && win != w && (win->windowFlags() & Qt::FramelessWindowHint)
                             && w->mapTo(win, QPoint(0, 0)).y() == 0;
            const bool underDecoration = !csdTop;
            const QRectF r(opt->rect);
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setPen(Qt::NoPen);
            p->setBrush(barBg(t));
            if (underDecoration) {
                p->drawRect(r);
            } else {
                const qreal rad = qMin<qreal>(15, r.height() / 2);
                QPainterPath path;
                path.addRoundedRect(r, rad, rad);
                path.addRect(r.adjusted(0, rad, 0, 0));  // square bottom corners
                path.setFillRule(Qt::WindingFill);
                p->drawPath(path.simplified());
            }
            p->restore();
            p->fillRect(QRect(opt->rect.left(), opt->rect.bottom(), opt->rect.width(), 1), barLine(t));
            return true;
        }
        if (c == "boxed-list") {  // AdwPreferencesGroup card: 12px radius, 1px ring (drawn inside our rect)
            p->save(); p->setRenderHint(QPainter::Antialiasing);
            p->setPen(Qt::NoPen);
            p->setBrush(listRing(t));
            p->drawRoundedRect(QRectF(opt->rect), t.radius_card, t.radius_card);
            p->setBrush(isDark(t) ? QColor(0x33, 0x33, 0x37) : QColor(Qt::white));  // card-bg (the token is off in hc-dark)
            p->drawRoundedRect(QRectF(opt->rect).adjusted(1, 1, -1, -1), t.radius_card - 1, t.radius_card - 1);
            p->restore(); return true;
        }
        if (c == "boxed-row") {  // separator on top of every row but the first
            if (w && w->property("adwaita-row-index").toInt() > 0)
                p->fillRect(QRect(opt->rect.left(), opt->rect.top(), opt->rect.width(), 1), listSep(t));
            return true;
        }
        return false;
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
        p->setPen(QPen(t.window_fg, 2.0, Qt::SolidLine, Qt::RoundCap));  // window-close-symbolic: bold, full colour
        const QPointF m = r.center();
        p->drawLine(m + QPointF(-3.3, -3.3), m + QPointF(3.3, 3.3));
        p->drawLine(m + QPointF(-3.3, 3.3), m + QPointF(3.3, -3.3));
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

// "&General" -> "General", "&&" -> "&" (the text a mnemonic label shows)
static QString plainText(const QString &s)
{
    QString o; o.reserve(s.size());
    for (int i = 0; i < s.size(); ++i) {
        if (s.at(i) == QLatin1Char('&')) { if (++i >= s.size()) break; }
        o.append(s.at(i));
    }
    return o;
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
        if (horizontal(tab->shape)) r = QRectF(r.left() + 3.5, r.top() + kTabInset, r.width() - 7, 34);
        else r.adjust(kTabInset, 3.5, -kTabInset, -3.5);
        qreal lv = sel ? (hover ? 1.0 : 0.8) : hover ? 0.5 : 0;
        if (down) lv = sel ? 1.5 : 1.2;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (lv > 0) {
            p->setPen(Qt::NoPen);
            p->setBrush(tabFill(t, lv));
            p->drawRoundedRect(r, 9, 9);
            if (isHc(t) && sel) {
                p->setBrush(Qt::NoBrush);
                p->setPen(QPen(isDark(t) ? QColor(0xa1, 0xa1, 0xa3) : QColor(0x8d, 0x8d, 0x8f), 1));
                p->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), 8.5, 8.5);
            }
        }
        p->restore();
        if (focusVisible(opt, w) && (tab->state & State_Selected)) drawFocusRing(p, r, 9);
        return true;
    }
    case CE_TabBarTabLabel: {
        auto *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
        if (!tab) return false;
        if (horizontal(tab->shape)) {  // AdwTabBar: title centred in the whole tab, the close button floats on the right
            p->save();
            // 26px sides leave room for a floating close button; without one 12px, so squeezed tabs keep their text.
            const int side = (tab->rightButtonSize.isValid() && tab->rightButtonSize.width() > 0)
                          || (tab->leftButtonSize.isValid() && tab->leftButtonSize.width() > 0) ? 26 : 12;
            const QRect tr = tab->rect.adjusted(side, 0, -side, 0);
            const int isz = 16;
            const bool hasIcon = !tab->icon.isNull();
            const QString plain = plainText(tab->text);
            const int avail = tr.width() - (hasIcon ? isz + 6 : 0);
            const bool fits = p->fontMetrics().horizontalAdvance(plain) <= avail;
            const int tw = qMin(p->fontMetrics().horizontalAdvance(plain), avail);
            const QString txt = fits ? tab->text : p->fontMetrics().elidedText(plain, Qt::ElideRight, avail);
            int tflags = Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine;
            if (fits) tflags |= Qt::TextShowMnemonic | (proxy()->styleHint(SH_UnderlineShortcut, opt, w) ? 0 : Qt::TextHideMnemonic);
            const int total = tw + (hasIcon ? isz + 6 : 0);
            int x = tr.center().x() - total / 2 + (total % 2 ? 0 : 0);
            if (hasIcon) {
                tab->icon.paint(p, QRect(x, tr.center().y() - isz / 2 + 1, isz, isz), Qt::AlignCenter,
                                (tab->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled);
                x += isz + 6;
            }
            p->setPen(withDisabled(t.window_fg, opt));
            p->drawText(QRect(x, tab->rect.top(), tw + 2, tab->rect.height() - 1), tflags, txt);
            p->restore();
            return true;
        }
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
            p->fillRect(opt->rect, barBg(t));
            p->fillRect(QRect(opt->rect.left(), opt->rect.bottom(), opt->rect.width(), 1), barLine(t));
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
            // Scroll areas stay square: their contents (viewport, headers, scroll bars) would cover rounded corners,
            // and clipping them in third-party apps proved unreliable.
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
        const bool circ = c.split(' ', Qt::SkipEmptyParts).contains("circular");
        const bool iconOnly = tb->text.isEmpty() || tb->toolButtonStyle == Qt::ToolButtonIconOnly;
        // libadwaita menu buttons with an icon (open-menu-symbolic) show no dropdown arrow
        const bool arrow = (tb->features & QStyleOptionToolButton::HasMenu) && !split && !iconOnly;
        const QRectF r(opt->rect);
        const qreal rad = (k == Pill || circ) ? r.height() / 2 : t.radius;
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
        } else if (circ && !(tb->state & (State_MouseOver | State_Sunken))) {
            p->setBrush(tabFill(t, 0.8));  // window-control circle: always filled
            p->drawRoundedRect(r, rad, rad);
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
        p->save();
        { QFont bf = p->font(); bf.setBold(true); p->setFont(bf); lo.font.setBold(true); }  // button label weight 700
        QCommonStyle::drawControl(CE_ToolButtonLabel, &lo, p, w);
        p->restore();
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
    case SE_TabBarTabRightButton: {
        auto *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
        if (!tab || !horizontal(tab->shape)) return false;
        const int sz = 20;  // glyph centre 16px inside the pill's right edge
        out = QRect(tab->rect.right() + 1 - 3 - 16 - sz / 2 - 1, tab->rect.top() + (tab->rect.height() - 1 - sz) / 2, sz, sz);
        return true;
    }
    case SE_TabBarTabLeftButton:
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
        if (tab && horizontal(tab->shape)) {
            s.setHeight(qMax(s.height(), kTabH));
            // The label is centred between 26px side paddings (room for the floating close button, see
            // CE_TabBarTabLabel); make the tab wide enough for the whole title unless the app squeezes it.
            const int icon = tab->icon.isNull() ? 0 : 16 + 6;
            const int side = (tab->rightButtonSize.width() > 0 || tab->leftButtonSize.width() > 0) ? 26 : 12;
            s.setWidth(qMax(s.width(), tab->fontMetrics.horizontalAdvance(plainText(tab->text)) + icon + 2 * side + 2));
        }
        else s.setWidth(qMax(s.width(), kTabH));
        out = s;
        return true;
    }
    case CT_ToolButton: {
        auto *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
        const bool hasText = tb && !tb->text.isEmpty() && tb->toolButtonStyle != Qt::ToolButtonIconOnly;
        int wd = sz.width() + (hasText ? 18 : 10);
        if (hasText) {  // label is drawn bold
            QFont bf = tb->font; bf.setBold(true);
            wd += QFontMetrics(bf).horizontalAdvance(tb->text) - tb->fontMetrics.horizontalAdvance(tb->text);
        }
        const bool circ = adwClass(w).split(' ', Qt::SkipEmptyParts).contains("circular");
        out = circ ? QSize(24, 24) : QSize(qMax(wd, 34), qMax(sz.height() + 10, 34));
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
