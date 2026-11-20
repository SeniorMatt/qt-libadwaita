// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: menus, menu bars, combo popups, tooltips (libadwaita popover.menu / tooltip, see docs/tokens.md sec. 5)
#include "adwaitastyle.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QImage>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionMenuItem>
#include <QWidget>

namespace {

constexpr int kShadow = 12;     // transparent margin around a popover that holds its shadow
constexpr int kMenuPad = 6;     // menu section padding
constexpr int kItemH = 32;
constexpr int kItemPadX = 12;
constexpr int kCheckCol = 24;   // check / radio indicator column

QColor alpha(QColor c, qreal a) { c.setAlphaF(a); return c; }

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

// Event filter that paints the popover background under a combo box popup container.
class PopupPainter : public QObject
{
public:
    PopupPainter(const AdwaitaTokens *t, QWidget *w) : QObject(w), m_t(t), m_w(w) {}
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
            const QRectF r = QRectF(m_w->rect()).adjusted(kShadow, kShadow, -kShadow, -kShadow);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.fillRect(m_w->rect(), Qt::transparent);
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            if (m_w->testAttribute(Qt::WA_TranslucentBackground)) {
                shadowLayer(&p, r, m_t->radius_popover, 2, 3, 14, 0.05);
                shadowLayer(&p, r, m_t->radius_popover, 1, 1, 5, 0.09);
                p.setPen(QPen(QColor(0, 0, 0, 13), 1)); p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(r.adjusted(-.5, -.5, .5, .5), m_t->radius_popover + .5, m_t->radius_popover + .5);
            }
            p.setPen(Qt::NoPen); p.setBrush(m_t->popover_bg);
            p.drawRoundedRect(r, m_t->radius_popover, m_t->radius_popover);
        }
        return false;
    }
private:
    const AdwaitaTokens *m_t;
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
        const QRectF r = QRectF(opt->rect).adjusted(m, m, -m, -m);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (translucent) {
            p->setCompositionMode(QPainter::CompositionMode_Source);
            p->fillRect(opt->rect, Qt::transparent);
            p->setCompositionMode(QPainter::CompositionMode_SourceOver);
            shadowLayer(p, r, t.radius_popover, 2, 3, 14, 0.05);
            shadowLayer(p, r, t.radius_popover, 1, 1, 5, 0.09);
            p->setPen(QPen(t.variant.startsWith("hc-") ? t.border : QColor(0, 0, 0, 13), 1));
            p->setBrush(Qt::NoBrush);
            p->drawRoundedRect(r.adjusted(-.5, -.5, .5, .5), t.radius_popover + .5, t.radius_popover + .5);
        }
        p->setPen(translucent ? Qt::NoPen : QPen(t.border, 1));
        p->setBrush(t.popover_bg);
        p->drawRoundedRect(r.adjusted(translucent ? 0 : .5, translucent ? 0 : .5, translucent ? 0 : -.5, translucent ? 0 : -.5),
                           t.radius_popover, t.radius_popover);
        p->restore();
        return true;
    }
    case PE_PanelTipLabel: {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(QColor(255, 255, 255, 26), 1));
        p->setBrush(t.tooltip_bg);
        p->drawRoundedRect(QRectF(opt->rect).adjusted(.5, .5, -.5, -.5), t.radius, t.radius);
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
            p->setBrush(alpha(t.popover_fg, (mi->state & State_Sunken) ? 0.19 : 0.10));
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
                p->drawPixmap(QRect(x, r.center().y() - is / 2 + 1, is, is),
                              mi->icon.pixmap(QSize(is, is), p->device()->devicePixelRatio(), mode));
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
    if (pm == PM_DefaultFrameWidth && isComboPopup(w)) { out = kShadow + kMenuPad; return true; }
    switch (pm) {
    case PM_MenuPanelWidth: out = kShadow; return true;
    case PM_MenuHMargin:
    case PM_MenuVMargin: out = kMenuPad; return true;
    case PM_MenuScrollerHeight: out = 16; return true;
    case PM_MenuTearoffHeight: out = 8; return true;
    case PM_SmallIconSize: out = 16; return true;
    case PM_MenuBarPanelWidth: out = 0; return true;
    case PM_MenuBarHMargin: out = 6; return true;
    case PM_MenuBarVMargin: out = 3; return true;
    case PM_MenuBarItemSpacing: out = 2; return true;
    case PM_ToolTipLabelFrameWidth: out = 7; return true;
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
        int w = sz.width() + 2 * kItemPadX;
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
        // A translucent window is needed for the rounded corners and the popover shadow. Without a
        // compositor (X11) the corners fall back to the window colour; set ADWAITA_NO_TRANSLUCENT=1 to force that.
        const bool translucent = qEnvironmentVariableIsEmpty("ADWAITA_NO_TRANSLUCENT");
        if (isTipLabel(w)) {
            if (translucent) { w->setAttribute(Qt::WA_TranslucentBackground, true); }
            return;
        }
        if (translucent) {
            w->setWindowFlag(Qt::NoDropShadowWindowHint, true);
            w->setAttribute(Qt::WA_TranslucentBackground, true);
        }
    }
    if (isComboPopup(w)) {
        w->installEventFilter(new PopupPainter(&m_tokens, w));
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
        w->setAttribute(Qt::WA_TranslucentBackground, false);
}
