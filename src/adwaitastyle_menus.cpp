// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: menus, menu bars, combo popups, tooltips (libadwaita popover.menu / tooltip, see docs/development/tokens.md sec. 5)
#include "adwaitastyle.h"
#include <QWindow>

#include <QAbstractItemView>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#ifdef ADWAITA_HAVE_XCB
#include <xcb/xcb.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/qguiapplication_platform.h>
#endif
#endif
#include <QComboBox>
#include <QEvent>
#include <QImage>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QStyleOptionMenuItem>
#include <QWidget>

namespace {

constexpr int kShadow = 12;     // transparent margin around a popover that holds its shadow
constexpr int kMenuPad = 6;     // menu section padding
constexpr int kItemH = 32;
constexpr int kItemPadX = 12;
constexpr int kCheckCol = 24;   // check / radio indicator column

// Measured from the libadwaita 1.10 reference shots (tests/visual/reference/<scheme>/*/menu@1x.png).
QColor menuRing(const AdwaitaTokens &t)
{
    if (t.variant == "hc-light") return QColor(0x9a, 0x9a, 0x9c);
    if (t.variant == "hc-dark") return QColor(0x9a, 0x9a, 0x9c);
    return t.variant == "dark" ? QColor(0x54, 0x54, 0x57) : QColor(0xe1, 0xe1, 0xe1);
}
// hover fill of popover/menu items: black .08 in light, white .10 in dark
qreal hoverA(const AdwaitaTokens &t, bool down) { const bool light = t.variant.endsWith("light"); return down ? (light ? 0.16 : 0.19) : (light ? 0.08 : 0.10); }

QColor alpha(QColor c, qreal a) { c.setAlphaF(a); return c; }

// Is a compositing manager running? Wayland and offscreen: always. X11: owner of _NET_WM_CM_S<screen>.
// ADWAITA_NO_TRANSLUCENT=1 forces "no" (manual override). Without xcb at build time we assume yes.
bool compositorActive()
{
    if (!qEnvironmentVariableIsEmpty("ADWAITA_NO_TRANSLUCENT")) return false;
    // In a Wayland session every window, XWayland ones included, goes through the compositor (KWin does not
    // always own _NET_WM_CM_S0 for XWayland, which made menus fall back to hard masked corners).
    if (qgetenv("XDG_SESSION_TYPE") == "wayland" || !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) return true;
#ifdef ADWAITA_HAVE_XCB
    if (QGuiApplication::platformName() == QLatin1String("xcb")) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        xcb_connection_t *c = nullptr;
        if (auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) c = x11->connection();
#else
        // Qt 5 has no public native interface for the connection: keep a small private one.
        static xcb_connection_t *c = xcb_connect(nullptr, nullptr);
        if (c && xcb_connection_has_error(c)) c = nullptr;
#endif
        {
            if (c) {
                int screen = 0;
                const QByteArray d = qgetenv("DISPLAY");
                const int dot = d.lastIndexOf('.');
                if (dot > d.lastIndexOf(':')) screen = d.mid(dot + 1).toInt();
                const QByteArray name = "_NET_WM_CM_S" + QByteArray::number(screen);
                auto ac = xcb_intern_atom_reply(c, xcb_intern_atom(c, true, name.size(), name.constData()), nullptr);
                if (!ac) return true;
                const xcb_atom_t atom = ac->atom;
                free(ac);
                if (atom == XCB_ATOM_NONE) return false;
                auto oc = xcb_get_selection_owner_reply(c, xcb_get_selection_owner(c, atom), nullptr);
                const bool on = oc && oc->owner != XCB_NONE;
                free(oc);
                return on;
            }
        }
    }
#endif
    return true;
}

bool isComboPopup(const QWidget *w);

// Popover shadow margin of a popup widget: kShadow when it is translucent, else the panel is a plain 1px frame.
int popupMargin(const QWidget *w)
{
    // QComboBox::showPopup sizes the container from metrics asked with the combo itself, so ask the compositor there.
    const bool tr = (w && !qobject_cast<const QComboBox *>(w) && !isComboPopup(w)) ? w->testAttribute(Qt::WA_TranslucentBackground) : compositorActive();
    return tr ? kShadow : 0;
}

bool isComboPopup(const QWidget *w)
{
    return w && QByteArray(w->metaObject()->className()) == "QComboBoxPrivateContainer";
}

bool isTipLabel(const QWidget *w)
{
    return w && QByteArray(w->metaObject()->className()) == "QTipLabel";
}

// Soft shadow built from stacked rounded rects (cheap approximation of a CSS box-shadow with blur).
void shadowLayer(QPainter *p, const QRectF &r, qreal rad, qreal dy, qreal spread, qreal blur, qreal a)
{
    p->setPen(Qt::NoPen);
    const int n = qMax(1, int(blur));
    for (int k = 0; k < n; ++k) {
        const qreal e = spread + blur / 2 - (k + 0.5) * blur / n;
        p->setBrush(QColor(0, 0, 0, int(255 * a * 2.0 / n + 0.5)));
        p->drawRoundedRect(r.translated(0, dy).adjusted(-e, -e, e, e), rad + e, rad + e);
    }
}

// Without a compositor a popup cannot be translucent, so it keeps its rounded corners with a window mask
// (like Breeze does): the corners are cut off, without shadow and without antialiasing at the edge.
class RoundMask : public QObject
{
public:
    RoundMask(QWidget *w, qreal radius) : QObject(w), m_w(w), m_r(radius) { setObjectName("adwRoundMask"); apply(); }
    void apply()
    {
        if (m_w->testAttribute(Qt::WA_TranslucentBackground)) { m_w->clearMask(); return; }
        QPainterPath path;
        path.addRoundedRect(QRectF(m_w->rect()), m_r, m_r);
        m_w->setMask(QRegion(path.toFillPolygon().toPolygon()));
    }
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o == m_w && (e->type() == QEvent::Resize || e->type() == QEvent::Show)) apply();
        return false;
    }
private:
    QWidget *m_w;
    qreal m_r;
};

// Event filter that paints the popover background under a combo box popup container.
class PopupPainter : public QObject
{
public:
    PopupPainter(const AdwaitaTokens *t, QWidget *w) : QObject(w), m_t(t), m_w(w) { setObjectName("adwPopupPainter"); }
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o == m_w && e->type() == QEvent::Show) {
            for (QAbstractItemView *v : m_w->findChildren<QAbstractItemView *>()) {
                v->setAutoFillBackground(false);
                v->viewport()->setAutoFillBackground(false);
            }
        }
        if (o == m_w && e->type() == QEvent::Paint) {
            QPainter p(m_w);
            p.setRenderHint(QPainter::Antialiasing);
            const bool tr = m_w->testAttribute(Qt::WA_TranslucentBackground);
            const int m = tr ? kShadow : 0;
            const QRectF r = QRectF(m_w->rect()).adjusted(m, m, -m, -m);
            if (tr) {
                p.setCompositionMode(QPainter::CompositionMode_Source);
                p.fillRect(m_w->rect(), Qt::transparent);
                p.setCompositionMode(QPainter::CompositionMode_SourceOver);
                shadowLayer(&p, r, m_t->radius_popover, 2, 3, 14, 0.05);
                shadowLayer(&p, r, m_t->radius_popover, 1, 1, 5, 0.09);
                p.setPen(QPen(QColor(0, 0, 0, 13), 1)); p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(r.adjusted(-.5, -.5, .5, .5), m_t->radius_popover + .5, m_t->radius_popover + .5);
                p.setPen(Qt::NoPen); p.setBrush(m_t->popover_bg);
                p.drawRoundedRect(r, m_t->radius_popover, m_t->radius_popover);
            } else {
                p.setPen(QPen(m_t->border, 1)); p.setBrush(m_t->popover_bg);
                p.drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), m_t->radius_popover - .5, m_t->radius_popover - .5);
            }
        }
        return false;
    }
private:
    const AdwaitaTokens *m_t;
    QWidget *m_w;
};

// Moves a translucent popup so the visible panel (not the shadow margin) lands where Qt placed the widget.
class PopupPlacer : public QObject
{
public:
    explicit PopupPlacer(QWidget *w) : QObject(w), m_w(w) { setObjectName("adwPopupPlacer"); }
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o != m_w) return false;
        if (e->type() == QEvent::Hide) { m_done = false; }
        // Some apps create the native window before the style is applied (Plasma calls winId() on its menus),
        // so the surface has no alpha and the corners/shadow cannot be translucent. The Show event comes before
        // the native window is mapped: drop it here and Qt recreates it with an alpha channel right away.
        if (e->type() == QEvent::Show && m_w->testAttribute(Qt::WA_TranslucentBackground)) {
            QWindow *wh = m_w->windowHandle();
            if (wh && wh->handle() && !wh->format().hasAlpha() && wh->requestedFormat().hasAlpha()) wh->destroy();
        }
        if (e->type() == QEvent::Show && !m_done && m_w->testAttribute(Qt::WA_TranslucentBackground)) {
            m_done = true;
            place();
        }
        return false;
    }
private:
    void place()
    {
        QRect g = m_w->geometry();
        if (auto *menu = qobject_cast<QMenu *>(m_w)) {
            QMenu *parent = nullptr;
            if (menu->menuAction())
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                for (QObject *ao : menu->menuAction()->associatedObjects())
#else
                for (QObject *ao : menu->menuAction()->associatedWidgets())
#endif
                    if (auto *pm = qobject_cast<QMenu *>(ao); pm && pm->isVisible() && pm != menu) parent = pm;
            if (parent) {  // submenu: panels sit side by side, y already follows Qt's item alignment
                const QRect pg = parent->geometry();
                const bool left = g.center().x() < pg.center().x();
                g.moveLeft(left ? pg.left() + kShadow - g.width() + kShadow : pg.right() + 1 - kShadow - kShadow);
            } else {
                g.translate(-kShadow, -kShadow);
            }
        } else if (auto *cb = qobject_cast<QComboBox *>(m_w->parentWidget())) {  // combo popup: panel = combo width, below/above it
            const QPoint top = cb->mapToGlobal(QPoint(0, 0));
            const bool above = g.center().y() < top.y();
            g = QRect(g.x() - kShadow, g.y() + (above ? kShadow : -kShadow), g.width() + 2 * kShadow, g.height());
        } else {
            return;
        }
        // keep the visible panel on screen
        if (QScreen *sc = QGuiApplication::screenAt(g.center())) {
            const QRect av = sc->availableGeometry().adjusted(-kShadow, -kShadow, kShadow, kShadow);
            if (g.right() > av.right()) g.moveRight(av.right());
            if (g.bottom() > av.bottom()) g.moveBottom(av.bottom());
            if (g.left() < av.left()) g.moveLeft(av.left());
            if (g.top() < av.top()) g.moveTop(av.top());
        }
        m_w->setGeometry(g);
    }
    QWidget *m_w;
    bool m_done = false;
};

// Tooltip: libadwaita padding is 6/10 (+1 border); QTipLabel only has a uniform margin, so add the horizontal rest.
class TipFixer : public QObject
{
public:
    explicit TipFixer(QWidget *w) : QObject(w), m_w(w) { setObjectName("adwTipFixer"); }
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o == m_w && (e->type() == QEvent::Show || e->type() == QEvent::Move || e->type() == QEvent::Resize)) {
            if (m_w->contentsMargins().left() != 4) m_w->setContentsMargins(4, 0, 4, 0);
            if (m_w->width() < m_w->sizeHint().width()) m_w->resize(m_w->sizeHint());
        }
        return false;
    }
private:
    QWidget *m_w;
};

void drawChevron(QPainter *p, const QPointF &c, bool right, const QColor &col)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(QPen(col, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->setBrush(Qt::NoBrush);
    if (right) p->drawPolyline(QPolygonF({c + QPointF(-1.5, -4), c + QPointF(2.5, 0), c + QPointF(-1.5, 4)}));
    else p->drawPolyline(QPolygonF({c + QPointF(-4, -1.5), c + QPointF(0, 2.5), c + QPointF(4, -1.5)}));
    p->restore();
}

}  // namespace

bool AdwaitaStyle::menus_drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (pe) {
    case PE_FrameMenu:
        return true;
    case PE_PanelMenu: {
        const bool translucent = w && w->testAttribute(Qt::WA_TranslucentBackground);
        const qreal m = translucent ? kShadow : 0;
        const qreal rad = translucent ? t.radius_popover : t.radius_popover - .5;  // masked (RoundMask) without a compositor
        const QRectF r = QRectF(opt->rect).adjusted(m, m, -m, -m);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (translucent) {
            p->setCompositionMode(QPainter::CompositionMode_Source);
            p->fillRect(opt->rect, Qt::transparent);
            p->setCompositionMode(QPainter::CompositionMode_SourceOver);
            shadowLayer(p, r, rad, 2, 3, 14, 0.05);
            shadowLayer(p, r, rad, 1, 1, 5, 0.09);
            p->setPen(QPen(menuRing(t), 1));
            p->setBrush(Qt::NoBrush);
            p->drawRoundedRect(r.adjusted(-.5, -.5, .5, .5), rad + .5, rad + .5);
        }
        p->setPen(translucent ? Qt::NoPen : QPen(menuRing(t), 1));
        p->setBrush(t.popover_bg);
        p->drawRoundedRect(r.adjusted(translucent ? 0 : .5, translucent ? 0 : .5, translucent ? 0 : -.5, translucent ? 0 : -.5),
                           rad, rad);
        p->restore();
        return true;
    }
    case PE_PanelTipLabel: {
        const bool tr = !isTipLabel(w) || w->testAttribute(Qt::WA_TranslucentBackground);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(QColor(255, 255, 255, 26), 1));
        p->setBrush(tr ? t.tooltip_bg : QColor::fromRgb(0x1c, 0x1c, 0x1e));  // opaque flat of rgba(0,0,6,.8) when not composited
        const qreal trad = tr ? t.radius : t.radius - .5;
        p->drawRoundedRect(QRectF(opt->rect).adjusted(.5, .5, -.5, -.5), trad, trad);
        p->restore();
        return true;
    }
    case PE_PanelMenuBar:
        p->fillRect(opt->rect, t.window_bg);
        return true;
    case PE_FrameStatusBarItem:
        return false;
    default: return false;
    }
}

bool AdwaitaStyle::menus_drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (ce) {
    case CE_MenuItem: {
        auto *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        if (!mi) return false;
        const QRect r = mi->rect;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        // The panel is only painted under the empty area, so each item paints the popover colour itself.
        p->fillRect(r, t.popover_bg);
        if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
            p->fillRect(QRect(r.left() - kMenuPad, r.center().y(), r.width() + 2 * kMenuPad, 1), t.border);
            p->restore();
            return true;
        }
        const bool enabled = mi->state & State_Enabled;
        const bool sel = (mi->state & State_Selected) && enabled;
        if (sel) {
            p->setPen(Qt::NoPen);
            p->setBrush(alpha(t.variant.endsWith("light") ? QColor(Qt::black) : QColor(Qt::white), hoverA(t, mi->state & State_Sunken)));
            p->drawRoundedRect(QRectF(r), t.radius, t.radius);
            if (t.variant.startsWith("hc-")) {
                p->setBrush(Qt::NoBrush); p->setPen(QPen(t.border, 1));
                p->drawRoundedRect(QRectF(r).adjusted(.5, .5, -.5, -.5), t.radius, t.radius);
            }
        }
        QColor fg = t.popover_fg;
        if (!enabled) fg.setAlphaF(fg.alphaF() * t.disabled_opacity);
        int x = r.left() + kItemPadX;
        const int right = r.right() + 1 - kItemPadX;
        // check / radio indicator column
        if (mi->menuHasCheckableItems) {
            if (mi->checked && mi->checkType != QStyleOptionMenuItem::NotCheckable) {
                const QPointF c(x + 7, r.center().y() + 0.5);
                p->setPen(QPen(fg, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                p->setBrush(Qt::NoBrush);
                if (mi->checkType == QStyleOptionMenuItem::Exclusive) {
                    p->setPen(Qt::NoPen); p->setBrush(fg);
                    p->drawEllipse(c, 3.0, 3.0);
                } else {
                    p->drawPolyline(QPolygonF({c + QPointF(-4.5, 0.5), c + QPointF(-1.5, 3.5), c + QPointF(4.5, -3.5)}));
                }
            }
            x += kCheckCol;
        }
        if (mi->maxIconWidth > 0) {
            if (!mi->icon.isNull()) {
                const int is = qMin(16, mi->maxIconWidth);
                const QIcon::Mode mode = enabled ? QIcon::Normal : QIcon::Disabled;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QPixmap ipm = mi->icon.pixmap(QSize(is, is), p->device()->devicePixelRatio(), mode);
#else
                const qreal idpr = p->device()->devicePixelRatioF();
                QPixmap ipm = mi->icon.pixmap(QSize(is, is) * idpr, mode);
                ipm.setDevicePixelRatio(idpr);
#endif
                p->drawPixmap(QRect(x, r.center().y() - is / 2 + 1, is, is), ipm);
            }
            x += mi->maxIconWidth + 8;
        }
        // submenu arrow
        int textRight = right;
        if (mi->menuItemType == QStyleOptionMenuItem::SubMenu) {
            drawChevron(p, QPointF(right - 8, r.center().y() + 0.5), true,
                        alpha(fg, fg.alphaF() * (t.variant.startsWith("hc-") ? 0.8 : 0.55)));
            textRight -= 20;
        }
        // text and shortcut
        QString text = mi->text, sc;
        const int tab = text.indexOf(QLatin1Char('\t'));
        if (tab >= 0) { sc = text.mid(tab + 1); text = text.left(tab); }
        int flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
        if (!proxy()->styleHint(SH_UnderlineShortcut, mi, w)) flags |= Qt::TextHideMnemonic;
        p->setFont(mi->font);
        if (mi->menuItemType == QStyleOptionMenuItem::DefaultItem) { QFont f = p->font(); f.setBold(true); p->setFont(f); }
        p->setPen(fg);
        p->drawText(QRect(x, r.top(), textRight - x, r.height()), flags | Qt::AlignLeft, text);
        if (!sc.isEmpty()) {
            p->setPen(alpha(fg, fg.alphaF() * (t.variant.startsWith("hc-") ? 0.9 : 0.55)));
            p->drawText(QRect(x, r.top(), textRight - x, r.height()), Qt::AlignVCenter | Qt::AlignRight | Qt::TextSingleLine, sc);
        }
        p->restore();
        return true;
    }
    case CE_ShapedFrame:
        return isComboPopup(w);
    case CE_MenuEmptyArea:
        return true;  // the rounded panel from PE_PanelMenu shows through
    case CE_MenuScroller:
    case CE_MenuTearoff:
        p->fillRect(opt->rect, t.popover_bg);
        return true;
    case CE_MenuBarEmptyArea:
        p->fillRect(opt->rect, t.window_bg);
        return true;
    case CE_MenuBarItem: {
        auto *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        if (!mi) return false;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->fillRect(opt->rect, t.window_bg);
        const bool enabled = mi->state & State_Enabled;
        if ((mi->state & (State_Selected | State_Sunken)) && enabled) {
            p->setPen(Qt::NoPen);
            p->setBrush(alpha(t.window_fg, (mi->state & State_Sunken) ? 0.16 : 0.10));
            p->drawRoundedRect(QRectF(opt->rect).adjusted(0, 1, 0, -1), t.radius, t.radius);
        }
        int flags = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
        if (!proxy()->styleHint(SH_UnderlineShortcut, mi, w)) flags |= Qt::TextHideMnemonic;
        p->setFont(mi->font);
        p->setPen(withDisabled(t.window_fg, opt));
        p->drawText(opt->rect, flags, mi->text);
        p->restore();
        return true;
    }
    default: return false;
    }
}

bool AdwaitaStyle::menus_drawComplexControl(ComplexControl, const QStyleOptionComplex *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::menus_subElementRect(SubElement, const QStyleOption *, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::menus_subControlRect(ComplexControl, const QStyleOptionComplex *, SubControl, const QWidget *, QRect &) const { return false; }

bool AdwaitaStyle::menus_pixelMetric(PixelMetric pm, const QStyleOption *, const QWidget *w, int &out) const
{
    if (pm == PM_DefaultFrameWidth && isComboPopup(w)) { out = popupMargin(w) + (popupMargin(w) ? 0 : 1) + kMenuPad; return true; }
    switch (pm) {
    case PM_MenuPanelWidth: out = popupMargin(w) ? kShadow : 1; return true;
    case PM_MenuHMargin:
    case PM_MenuVMargin: out = kMenuPad; return true;
    case PM_MenuScrollerHeight: out = 16; return true;
    case PM_MenuTearoffHeight: out = 8; return true;
    case PM_SmallIconSize: out = 16; return true;
    case PM_MenuBarPanelWidth: out = 0; return true;
    case PM_MenuBarHMargin: out = 6; return true;
    case PM_MenuBarVMargin: out = 3; return true;
    case PM_MenuBarItemSpacing: out = 2; return true;
    case PM_ToolTipLabelFrameWidth: out = 6; return true;  // + margin 1 = 7 (6 padding + 1 border); 4 more each side via TipFixer
    default: break;
    }
    Q_UNUSED(w);
    return false;
}

bool AdwaitaStyle::menus_sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &sz, const QWidget *, QSize &out) const
{
    if (ct == CT_MenuItem) {
        auto *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        if (!mi) return false;
        if (mi->menuItemType == QStyleOptionMenuItem::Separator) { out = QSize(sz.width(), 13); return true; }
        int w = sz.width() + 2 * kItemPadX - 2;  // -2: measured against the reference popover (128 vs 130 wide)
        if (mi->menuHasCheckableItems) w += kCheckCol;
        if (mi->maxIconWidth > 0) w += mi->maxIconWidth + 8;
        if (mi->menuItemType == QStyleOptionMenuItem::SubMenu) w += 20;
        if (mi->text.contains(QLatin1Char('\t'))) w += 12;
        out = QSize(qMax(w, 120 - 2 * kMenuPad), qMax(sz.height() + 10, kItemH));
        return true;
    }
    if (ct == CT_MenuBarItem) { out = QSize(sz.width() + 16, qMax(sz.height() + 8, 30)); return true; }
    return false;
}

bool AdwaitaStyle::menus_styleHint(StyleHint sh, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &out) const
{
    switch (sh) {
    case SH_Menu_MouseTracking:
    case SH_MenuBar_MouseTracking:
    case SH_ComboBox_ListMouseTracking: out = 1; return true;  // QCommonStyle says 0: no hover highlight
    case SH_Menu_SubMenuPopupDelay: out = 200; return true;
    case SH_Menu_Scrollable: out = 1; return true;
    case SH_Menu_SloppySubMenus: out = 1; return true;
    case SH_Menu_KeyboardSearch: out = 1; return true;
    case SH_Menu_AllowActiveAndDisabled: out = 0; return true;
    case SH_ComboBox_PopupFrameStyle: out = QFrame::StyledPanel | QFrame::Plain; return true;
    case SH_ToolTip_Mask: return false;
    default: return false;
    }
}

void AdwaitaStyle::menus_polish(QWidget *w)
{
    if (qobject_cast<QMenu *>(w) || isComboPopup(w) || isTipLabel(w)) {
        // A translucent window gives the rounded corners and the popover shadow. Without a compositor it would
        // be black/opaque, so menus and tooltips become square, shadowless and opaque instead.
        const bool translucent = compositorActive();
        if (translucent) {
            w->setWindowFlag(Qt::NoDropShadowWindowHint, true);
            w->setAttribute(Qt::WA_TranslucentBackground, true);
            if (!isTipLabel(w) && !w->findChild<QObject *>("adwPopupPlacer")) w->installEventFilter(new PopupPlacer(w));
            if (isComboPopup(w)) { QEvent se(QEvent::StyleChange); QCoreApplication::sendEvent(w, &se); }  // refresh the cached frame width (margin now 18)
        }
        if (!translucent && !w->findChild<QObject *>("adwRoundMask"))
            w->installEventFilter(new RoundMask(w, isTipLabel(w) ? m_tokens.radius : m_tokens.radius_popover));
        if (isTipLabel(w) && !w->findChild<QObject *>("adwTipFixer")) w->installEventFilter(new TipFixer(w));
        if (isTipLabel(w)) return;
    }
    if (isComboPopup(w)) {
        if (!w->findChild<QObject *>("adwPopupPainter")) w->installEventFilter(new PopupPainter(&m_tokens, w));
        for (QWidget *c : w->findChildren<QWidget *>()) {
            if (c->metaObject()->className() == QByteArray("QComboBoxListView") || c->inherits("QAbstractItemView")) {
                c->setAutoFillBackground(false);
                if (auto *v = qobject_cast<QWidget *>(c->parent())) if (v != w) v->setAutoFillBackground(false);
            }
        }
    }
}

void AdwaitaStyle::menus_unpolish(QWidget *w)
{
    if (qobject_cast<QMenu *>(w) || isComboPopup(w) || isTipLabel(w))
    {
        w->setAttribute(Qt::WA_TranslucentBackground, false);
        if (QObject *m = w->findChild<QObject *>("adwRoundMask")) { w->removeEventFilter(m); delete m; w->clearMask(); }
    }
}
