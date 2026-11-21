// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: item views (list, tree, table), header views, tree branches
#include "adwaitastyle.h"
#include "adwaitaanimations.h"

#include <QAbstractItemView>
#include <QPointer>
#include <QHeaderView>
#include <QListView>
#include <QPainter>
#include <QTableView>
#include <QTreeView>
#include <QAbstractScrollArea>
#include <QEvent>
#include <QPainterPath>
#include <QRegion>
#include <QWidget>
#include <QStyleOptionHeader>
#include <QStyleOptionViewItem>

namespace {

constexpr int kRowMin = 32;  // combo popup rows (menu item height)

QColor alpha(QColor c, qreal a) { c.setAlphaF(a); return c; }

bool inComboPopup(const QWidget *w)
{
    for (const QWidget *p = w ? w->parentWidget() : nullptr; p; p = p->parentWidget())
        if (QByteArray(p->metaObject()->className()) == "QComboBoxPrivateContainer") return true;
    return false;
}

}  // namespace

// KIO's places view (Dolphin's Places panel, KDE file dialogs) paints its icon 4 px from the item edge, which put
// the icon on the rim of our inset pill. Its delegate calls PE_PanelItemViewItem first, inside its own
// painter save()/restore(), and paints icon and text afterwards with the same painter; so after drawing the pill
// we translate the painter, which moves KIO's icon and text inward, and KIO's restore() undoes it.
// (Never replace that delegate: KIO casts view->itemDelegate() to its own class without a check and crashes.)
static constexpr int kPlacesShift = 6;

static bool s_rowInner = false;  // QCommonStyle re-enters PE_PanelItemViewItem with the state stripped: no transition then

static quint32 indexKey(const QModelIndex &ix)
{
    return quint32(ix.row()) * 2654435761u ^ quint32(ix.column() + 1) * 40503u ^ quint32(quintptr(ix.internalId()) * 2246822519u);
}

bool AdwaitaStyle::views_drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (pe) {
    case PE_PanelItemViewRow:
        return true;  // highlight is painted per cell (and in the branch area), never twice
    case PE_PanelItemViewItem: {
        auto *v = qstyleoption_cast<const QStyleOptionViewItem *>(opt);
        if (!v) return false;
        const bool sel = v->state & State_Selected;
        const bool hover = (v->state & State_MouseOver) && (v->state & State_Enabled);
        const bool fromKio = !w;  // KIO's places delegate passes no widget
        if (!w) w = v->widget;
        const bool combo = inComboPopup(w);
        const bool places = fromKio && w && w->inherits("KFilePlacesView") && v->rect.width() > 40;
        // libadwaita: sidebar / list rows are rounded pills (radius 9) inset a little; trees and tables (column
        // views) keep full-width rows, rounded only at the row ends.
        const bool columns = qobject_cast<const QTreeView *>(w) || qobject_cast<const QTableView *>(w);
        const qreal rad = t.radius;
        QRectF r(v->rect);
        QPainterPath shape;
        if (columns) {
            const auto pos = v->viewItemPosition;
            const bool left = pos == QStyleOptionViewItem::Beginning || pos == QStyleOptionViewItem::OnlyOne;
            const bool right = pos == QStyleOptionViewItem::End || pos == QStyleOptionViewItem::OnlyOne;
            const auto *tv = qobject_cast<const QTableView *>(w);
            const bool table = tv;
            if (table && !tv->showGrid()) {
                // Table cells without grid lines are rounded like list rows (9 px), squared on the sides where the
                // neighbouring cell is selected too, so a row selection reads as one bar. Tables with grid lines
                // stay square: rounded fills would break the lines they sit between.
                r.adjust(0, 1, 0, -1);
                bool selL = false, selR = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                if (sel && tv->selectionModel() && v->index.isValid()) {
                    const QModelIndex ix = v->index;
                    selL = tv->selectionModel()->isSelected(ix.sibling(ix.row(), ix.column() - 1)) && ix.column() > 0;
                    selR = tv->selectionModel()->isSelected(ix.sibling(ix.row(), ix.column() + 1));
                }
#endif
                shape.addRoundedRect(r, rad, rad);
                if (selL) shape.addRect(QRectF(r.left(), r.top(), qMin(rad, r.width()), r.height()));
                if (selR) shape.addRect(QRectF(r.right() - qMin(rad, r.width()), r.top(), qMin(rad, r.width()), r.height()));
                shape.setFillRule(Qt::WindingFill);
                shape = shape.simplified();
            }
            else if (table || pos == QStyleOptionViewItem::Invalid) { shape.addRect(r); }
            else {
                // rounded rect, then square off the inner ends
                shape.addRoundedRect(r, rad, rad);
                if (!left) shape.addRect(QRectF(r.left(), r.top(), qMin(rad, r.width()), r.height()));
                if (!right) shape.addRect(QRectF(r.right() - qMin(rad, r.width()), r.top(), qMin(rad, r.width()), r.height()));
                shape.setFillRule(Qt::WindingFill);
                shape = shape.simplified();
            }
        } else if (places) {
            r.adjust(3, 1, -3, -1);
            shape.addRoundedRect(r, rad, rad);
        } else {
            if (!combo && r.width() > 24 && r.height() > 12) r.adjust(r.width() > 120 ? 6 : 2, 1, r.width() > 120 ? -6 : -2, -1);
            shape.addRoundedRect(r, rad, rad);
        }
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        if (v->backgroundBrush.style() != Qt::NoBrush) {
            p->setBrush(v->backgroundBrush);
            p->drawPath(shape);
        }
        QColor fill;
        if (combo) {
            if (sel || hover) fill = alpha(t.popover_fg, sel && hover ? 0.13 : 0.10);
        } else if (sel) {
            fill = alpha(t.accent_bg, hover ? 0.32 : 0.25);
        } else if (hover) {
            fill = alpha(t.view_fg, 0.08);  // .04 was too faint to notice
        }
        // row background fades between hover / selected states (200 ms), one entry per model index
        if (w && !s_rowInner && v->index.isValid() && (v->state & State_Enabled))
            fill = AdwAnim::color(w, AdwAnim::key(AdwAnim::Row, indexKey(v->index)), fill.isValid() ? fill : QColor(Qt::transparent));
        if (fill.isValid() && fill.alpha() > 0) {
            p->setBrush(fill);
            p->drawPath(shape);
        }
        p->restore();
        if (places) p->translate(v->direction == Qt::RightToLeft ? -kPlacesShift : kPlacesShift, 0);
        return true;
    }
    case PE_IndicatorBranch: {
        const QRect r = opt->rect;
        const bool sel = opt->state & State_Selected;
        const bool hover = opt->state & State_MouseOver;
        // the model index of this row (branch area = left part of column 0), for per-row transitions
        quint32 ik = 0;
        const QTreeView *tv = qobject_cast<const QTreeView *>(w);
        if (tv) { const QModelIndex ix = tv->indexAt(QPoint(tv->columnViewportPosition(0) + 1, r.center().y())); if (ix.isValid()) ik = indexKey(ix); }
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        QColor bg = sel ? alpha(t.accent_bg, hover ? 0.32 : 0.25) : hover ? alpha(t.view_fg, 0.08) : QColor(Qt::transparent);
        if (ik) bg = AdwAnim::color(w, AdwAnim::key(AdwAnim::Branch, ik), bg);
        if (bg.alpha() > 0) p->fillRect(r, bg);
        if (opt->state & State_Children) {
            const bool open = opt->state & State_Open;
            QColor c = t.view_fg;
            c.setAlphaF(c.alphaF() * 0.7);
            p->setPen(QPen(c, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p->setBrush(Qt::NoBrush);
            const QPointF m = QRectF(r).center();
            const qreal ex = ik ? AdwAnim::running(w, AdwAnim::key(AdwAnim::Expand, ik), open ? 1.0 : 0.0) : (open ? 1.0 : 0.0);
            if (ex > 0 && ex < 1) {  // expander arrow turning (right -> down) while a row opens / closes
                p->translate(m);
                p->rotate(90 * ex);
                p->drawPolyline(QPolygonF({QPointF(-1.5, -3.5), QPointF(2, 0), QPointF(-1.5, 3.5)}));
            } else if (open) p->drawPolyline(QPolygonF({m + QPointF(-3.5, -1.5), m + QPointF(0, 2), m + QPointF(3.5, -1.5)}));
            else p->drawPolyline(QPolygonF({m + QPointF(-1.5, -3.5), m + QPointF(2, 0), m + QPointF(-1.5, 3.5)}));
        }
        p->restore();
        return true;
    }
    case PE_IndicatorHeaderArrow: {
        auto *h = qstyleoption_cast<const QStyleOptionHeader *>(opt);
        if (!h) return false;
        const bool up = h->sortIndicator == QStyleOptionHeader::SortUp;
        if (h->sortIndicator == QStyleOptionHeader::None) return true;
        QColor c = t.view_fg;
        c.setAlphaF(c.alphaF() * 0.7);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(c, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF m = QRectF(opt->rect).center();
        if (up) p->drawPolyline(QPolygonF({m + QPointF(-3.5, 1.5), m + QPointF(0, -2), m + QPointF(3.5, 1.5)}));
        else p->drawPolyline(QPolygonF({m + QPointF(-3.5, -1.5), m + QPointF(0, 2), m + QPointF(3.5, -1.5)}));
        p->restore();
        return true;
    }
    default: return false;
    }
}

bool AdwaitaStyle::views_drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (ce) {
    case CE_RubberBand: {
        // libadwaita: rubberband { border: 1px solid accent_color; background-color: alpha(accent_color, .2) }
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(t.accent_color, 1));
        p->setBrush(alpha(t.accent_color, 0.2));
        p->drawRoundedRect(QRectF(opt->rect).adjusted(.5, .5, -.5, -.5), 3, 3);
        p->restore();
        return true;
    }
    case CE_ItemViewItem: {
        auto *v = qstyleoption_cast<const QStyleOptionViewItem *>(opt);
        if (!v) return false;
        // background first (with the selected / hover state), then text and icon in the plain colours:
        // libadwaita keeps the foreground on selected rows, it only tints the background.
        drawPrimitive(PE_PanelItemViewItem, opt, p, w);
        QStyleOptionViewItem o = *v;
        const bool combo = inComboPopup(w);
        o.state &= ~(State_Selected | State_MouseOver);
        o.backgroundBrush = Qt::NoBrush;
        if (!o.icon.isNull()) o.icon = symbolicIcon(o.icon, combo ? t.popover_fg : t.view_fg);
        const QPalette::ColorGroup g = (o.state & State_Enabled) ? QPalette::Normal : QPalette::Disabled;
        o.palette.setColor(g, QPalette::Text, combo ? t.popover_fg : t.view_fg);
        if (combo && (v->state & State_Selected)) o.state |= State_Selected;  // keep the combo's current-item look
        if (combo) o.state &= ~State_Selected;
        const bool ring = focusVisible(opt, w) && (v->state & State_HasFocus);
        o.state &= ~State_HasFocus;
        s_rowInner = true;
        QCommonStyle::drawControl(CE_ItemViewItem, &o, p, w);
        s_rowInner = false;
        if (ring) drawFocusRing(p, QRectF(v->rect).adjusted(0, 0, 0, 0), 0);
        return true;
    }
    case CE_HeaderSection: {
        auto *h = qstyleoption_cast<const QStyleOptionHeader *>(opt);
        if (!h) return false;
        const QRect r = opt->rect;
        p->save();
        p->fillRect(r, t.view_bg);
        const quint32 sec = quint32(h->section + 1);
        const QColor hov = AdwAnim::color(w, AdwAnim::key(AdwAnim::Fill, sec),
                                          (opt->state & State_MouseOver && (opt->state & State_Enabled)) ? alpha(t.view_fg, 0.04) : QColor(Qt::transparent));
        const QColor prs = AdwAnim::color(w, AdwAnim::key(AdwAnim::Fill2, sec),
                                          (opt->state & State_Sunken) ? alpha(t.view_fg, 0.08) : QColor(Qt::transparent));
        if (hov.alpha() > 0) p->fillRect(r, hov);
        if (prs.alpha() > 0) p->fillRect(r, prs);
        const bool horizontal = h->orientation == Qt::Horizontal;
        if (horizontal) {
            p->fillRect(QRect(r.left(), r.bottom(), r.width(), 1), t.border);
            // libadwaita column headers have no separators at rest; the section edge shows a short subtle one while the
            // section is hovered or pressed (resize affordance)
            if (h->position != QStyleOptionHeader::End && h->position != QStyleOptionHeader::OnlyOneSection
                && (opt->state & (State_MouseOver | State_Sunken)) && (opt->state & State_Enabled))
                p->fillRect(QRect(r.right(), r.top() + 8, 1, r.height() - 17), t.border);
        } else {
            p->fillRect(QRect(r.right(), r.top(), 1, r.height()), t.border);
            p->fillRect(QRect(r.left() + 6, r.bottom(), r.width() - 13, 1), t.border);
        }
        p->restore();
        return true;
    }
    case CE_HeaderLabel: {
        auto *h = qstyleoption_cast<const QStyleOptionHeader *>(opt);
        if (!h) return false;
        QStyleOptionHeader o = *h;
        QColor c = t.view_fg;
        c.setAlphaF(c.alphaF() * (t.variant.startsWith("hc-") ? 0.9 : 0.75));
        o.palette.setColor(QPalette::ButtonText, withDisabled(c, opt));
        o.palette.setColor(QPalette::WindowText, withDisabled(c, opt));
        p->save();
        QFont f = p->font(); f.setBold(true); p->setFont(f);
        o.fontMetrics = QFontMetrics(f);
        QCommonStyle::drawControl(CE_HeaderLabel, &o, p, w);
        p->restore();
        return true;
    }
    case CE_Header:
        return false;  // default: section + label via the hooks above
    default: return false;
    }
}

bool AdwaitaStyle::views_drawComplexControl(ComplexControl, const QStyleOptionComplex *, QPainter *, const QWidget *) const { return false; }
// Inner padding at the ends of a rounded row so icons and text sit inside the pill, not on its edge
// (libadwaita rows: 6px pill inset + ~9px content padding). Returns {left, right}.
static QPair<int, int> rowEndPadding(const QStyleOptionViewItem *v, const QWidget *w)
{
    if (inComboPopup(w)) return {0, 0};
    const bool columns = qobject_cast<const QTreeView *>(w) || qobject_cast<const QTableView *>(w);
    if (qobject_cast<const QTableView *>(w)) return {0, 0};
    if (columns) {
        const auto pos = v->viewItemPosition;
        if (pos == QStyleOptionViewItem::Invalid) return {0, 0};
        const bool left = pos == QStyleOptionViewItem::Beginning || pos == QStyleOptionViewItem::OnlyOne;
        const bool right = pos == QStyleOptionViewItem::End || pos == QStyleOptionViewItem::OnlyOne;
        return {left ? 6 : 0, right ? 6 : 0};
    }
    if (v->decorationPosition == QStyleOptionViewItem::Top || v->decorationPosition == QStyleOptionViewItem::Bottom)
        return {0, 0};  // icon views: content is centred
    const int inset = v->rect.width() > 120 ? 6 : 2;  // must match the pill inset in PE_PanelItemViewItem
    return {inset + 6, inset + 6};
}

bool AdwaitaStyle::views_subElementRect(SubElement se, const QStyleOption *opt, const QWidget *w, QRect &out) const
{
    if (se == SE_ItemViewItemCheckIndicator || se == SE_ItemViewItemDecoration || se == SE_ItemViewItemText
        || se == SE_ItemViewItemFocusRect) {
        auto *v = qstyleoption_cast<const QStyleOptionViewItem *>(opt);
        if (!v) return false;
        const auto pad = rowEndPadding(v, w);
        if (!pad.first && !pad.second) return false;
        QStyleOptionViewItem o = *v;
        const bool rtl = o.direction == Qt::RightToLeft;
        o.rect.adjust(rtl ? pad.second : pad.first, 0, rtl ? -pad.first : -pad.second, 0);
        if (o.rect.width() < 1) return false;
        out = QCommonStyle::subElementRect(se, &o, w);
        return true;
    }
    if (se == SE_HeaderArrow) {
        const QRect r = opt->rect;
        out = QRect(r.right() - 9 - 10, r.center().y() - 5, 10, 10);
        return true;
    }
    return false;
}
bool AdwaitaStyle::views_subControlRect(ComplexControl, const QStyleOptionComplex *, SubControl, const QWidget *, QRect &) const { return false; }

bool AdwaitaStyle::views_pixelMetric(PixelMetric pm, const QStyleOption *, const QWidget *w, int &out) const
{
    switch (pm) {
    case PM_HeaderMargin: out = 9; return true;
    case PM_HeaderMarkSize: out = 10; return true;
    case PM_HeaderGripMargin: out = 4; return true;
    case PM_TreeViewIndentation: out = 16; return true;
    case PM_FocusFrameHMargin:  // item views: Qt's text margin is this + 1, it is also the gap between check box, icon and text
        out = (qobject_cast<const QAbstractItemView *>(w) && !inComboPopup(w)) ? 2 : 0; return true;
    case PM_FocusFrameVMargin: out = 0; return true;
    default: return false;
    }
}

bool AdwaitaStyle::views_sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &sz, const QWidget *w, QSize &out) const
{
    if (ct == CT_HeaderSection) {
        QSize s = QCommonStyle::sizeFromContents(ct, opt, sz, w);
        out = QSize(s.width(), qMax(s.height(), 34));
        return true;
    }
    if (ct == CT_ItemViewItem) {
        QSize s = QCommonStyle::sizeFromContents(ct, opt, sz, w);
        const qreal k = inComboPopup(w) ? 1.0 : adwSpacingScale();  // user spacing option makes rows taller/shorter
        int padW = 0;
        if (auto *v = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            const auto pad = rowEndPadding(v, w);
            padW = pad.first + pad.second;
        }
        out = QSize(s.width() + padW, qMax(s.height() + qRound(6 * k), inComboPopup(w) ? kRowMin : qRound(34 * qMin<qreal>(k, 1.0) + 12 * qMax<qreal>(k - 1, 0))));
        return true;
    }
    return false;
}

bool AdwaitaStyle::views_styleHint(StyleHint sh, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &out) const
{
    switch (sh) {
    case SH_ItemView_ShowDecorationSelected: out = 1; return true;
    case SH_RubberBand_Mask: out = 0; return true;  // filled band, not QCommonStyle's hollow outline
    case SH_ItemView_ChangeHighlightOnFocus: out = 0; return true;
    case SH_ItemView_PaintAlternatingRowColorsForEmptyArea: out = 0; return true;
    case SH_Table_GridLineColor: out = int(m_tokens.border.rgba()); return true;
    default: return false;
    }
}




void AdwaitaStyle::views_polish(QWidget *w)
{
    if (auto *v = qobject_cast<QAbstractItemView *>(w)) {
        v->viewport()->setAttribute(Qt::WA_Hover, true);
        v->setMouseTracking(true);
        if (auto *tv = qobject_cast<QTableView *>(v)) tv->setShowGrid(false);
        if (auto *tr = qobject_cast<QTreeView *>(v)) {
            static const char kExp[] = "adwExpanderAnim";
            if (!tr->findChild<QObject *>(kExp, Qt::FindDirectChildrenOnly)) {  // plain QObject: found by name
                auto *ctx = new QObject(tr);
                ctx->setObjectName(kExp);
                auto start = [tr](const QModelIndex &ix, bool open) {
                    AdwAnim::kick(tr, AdwAnim::key(AdwAnim::Expand, indexKey(ix)), open ? 0.0 : 1.0, open ? 1.0 : 0.0);
                };
                connect(tr, &QTreeView::expanded, ctx, [start](const QModelIndex &ix) { start(ix, true); });
                connect(tr, &QTreeView::collapsed, ctx, [start](const QModelIndex &ix) { start(ix, false); });
            }
        }
    }
}

void AdwaitaStyle::views_unpolish(QWidget *w)
{
    if (auto *tr = qobject_cast<QTreeView *>(w)) delete tr->findChild<QObject *>("adwExpanderAnim", Qt::FindDirectChildrenOnly);
}
