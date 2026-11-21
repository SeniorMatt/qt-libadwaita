// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: calendar, tool box, MDI sub-window title bars, dials, dialog / message box icons, glyph icons,
// command link buttons, tool buttons with text under the icon. Listed first in ADW_FAMILIES so it sees an element
// before the older families do.
#include "adwaitastyle.h"

#include <QAbstractButton>
#include <QApplication>
#include <QCalendarWidget>
#include <QCommandLinkButton>
#include <QDial>
#include <QFontMetrics>
#include <QIconEngine>
#include <QMdiArea>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QTextCharFormat>
#include <QStyleOptionComplex>
#include <QStyleOptionSlider>
#include <QStyleOptionTitleBar>
#include <QStyleOptionToolBox>
#include <QStyleOptionToolButton>
#include <QTableView>
#include <QToolBox>
#include <QToolButton>
#include <cmath>

namespace {

QColor alphaOf(QColor c, qreal a) { c.setAlphaF(c.alphaF() * a); return c; }

const AdwaitaTokens *tokens()
{
    auto *s = qobject_cast<AdwaitaStyle *>(QApplication::style());
    return s ? &s->tokens() : nullptr;
}

// ---- glyph icons -----------------------------------------------------------------------------------------

enum Glyph { GCheck, GCross, GPrev, GNext, GRestore, GMin, GMax, GHelp, GShadeUp, GShadeDown, GInfo, GQuestion, GWarning, GCritical };

// Draws a glyph on a 16x16 grid scaled into r; fg is the colour of the line glyphs. The message glyphs are
// filled shapes in the accent / warning / error colours, like the coloured symbolic icons of GNOME.
void drawGlyph(QPainter *p, Glyph g, const QRectF &r, const QColor &fg)
{
    const AdwaitaTokens *t = tokens();
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    const qreal s = qMin(r.width(), r.height()) / 16.0;
    p->translate(r.center() - QPointF(8 * s, 8 * s));
    p->scale(s, s);
    QPen pen(fg, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p->setBrush(Qt::NoBrush);
    auto disc = [&](const QColor &c) { p->setPen(Qt::NoPen); p->setBrush(c); p->drawEllipse(QRectF(0.5, 0.5, 15, 15)); };
    const QColor white(255, 255, 255);
    switch (g) {
    case GCheck: p->setPen(pen); p->drawPolyline(QPolygonF({{3.5, 8.5}, {6.7, 11.7}, {12.5, 4.8}})); break;
    case GCross: p->setPen(pen); p->drawLine(QPointF(4, 4), QPointF(12, 12)); p->drawLine(QPointF(12, 4), QPointF(4, 12)); break;
    case GPrev: p->setPen(pen); p->drawPolyline(QPolygonF({{10, 3.5}, {5.5, 8}, {10, 12.5}})); break;
    case GNext: p->setPen(pen); p->drawPolyline(QPolygonF({{6, 3.5}, {10.5, 8}, {6, 12.5}})); break;
    case GShadeUp: p->setPen(pen); p->drawPolyline(QPolygonF({{3.5, 10}, {8, 5.5}, {12.5, 10}})); break;
    case GShadeDown: p->setPen(pen); p->drawPolyline(QPolygonF({{3.5, 6}, {8, 10.5}, {12.5, 6}})); break;
    case GMin: p->setPen(pen); p->drawLine(QPointF(3.5, 11), QPointF(12.5, 11)); break;
    case GMax: p->setPen(pen); p->drawRoundedRect(QRectF(3.5, 3.5, 9, 9), 1.5, 1.5); break;
    case GRestore:
        p->setPen(pen);
        p->drawRoundedRect(QRectF(3.2, 6.2, 6.6, 6.6), 1.3, 1.3);
        p->drawPolyline(QPolygonF({{6.2, 6.2}, {6.2, 3.2}, {12.8, 3.2}, {12.8, 9.8}, {9.8, 9.8}}));
        break;
    case GHelp: {
        p->setPen(QPen(fg, 1.4));
        p->drawEllipse(QRectF(1.5, 1.5, 13, 13));
        QPainterPath q; QFont f; f.setPixelSize(10); f.setBold(true);
        q.addText(0, 0, f, QStringLiteral("?"));
        const QRectF b = q.boundingRect();
        p->setPen(Qt::NoPen); p->setBrush(fg);
        p->drawPath(q.translated(8 - b.center().x(), 8 - b.center().y()));
        break;
    }
    case GInfo:
        disc(t ? t->accent_bg : QColor(0x35, 0x84, 0xe4));
        p->setBrush(white); p->drawEllipse(QPointF(8, 4.9), 1.15, 1.15);
        p->setPen(QPen(white, 1.9, Qt::SolidLine, Qt::RoundCap)); p->drawLine(QPointF(8, 7.4), QPointF(8, 11.6));
        break;
    case GQuestion: {
        disc(t ? t->accent_bg : QColor(0x35, 0x84, 0xe4));
        QPainterPath q; QFont f; f.setPixelSize(11); f.setBold(true);
        q.addText(0, 0, f, QStringLiteral("?"));
        const QRectF b = q.boundingRect();
        p->setPen(Qt::NoPen); p->setBrush(white);
        p->drawPath(q.translated(8 - b.center().x(), 8 - b.center().y()));
        break;
    }
    case GWarning: {
        const QColor c = t ? t->warning_color : QColor(0xf5, 0xc2, 0x11);
        p->setPen(QPen(c, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p->setBrush(c);
        p->drawPolygon(QPolygonF({{8, 2.4}, {14.2, 13.2}, {1.8, 13.2}}));
        p->setPen(QPen(QColor(0x24, 0x1f, 0x31), 1.7, Qt::SolidLine, Qt::RoundCap)); p->drawLine(QPointF(8, 6.4), QPointF(8, 9.6));
        p->setPen(Qt::NoPen); p->setBrush(QColor(0x24, 0x1f, 0x31)); p->drawEllipse(QPointF(8, 11.5), 1.0, 1.0);
        break;
    }
    case GCritical:
        disc(t ? t->error_color : QColor(0xe0, 0x1b, 0x24));
        p->setPen(QPen(white, 1.9, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(5.2, 5.2), QPointF(10.8, 10.8)); p->drawLine(QPointF(10.8, 5.2), QPointF(5.2, 10.8));
        break;
    }
    p->restore();
}

class GlyphEngine : public QIconEngine
{
public:
    explicit GlyphEngine(Glyph g) : m_g(g) {}
    void paint(QPainter *p, const QRect &rect, QIcon::Mode mode, QIcon::State) override
    {
        QColor fg = QApplication::palette().color(QPalette::ButtonText);
        if (mode == QIcon::Disabled) fg.setAlphaF(fg.alphaF() * 0.5);
        drawGlyph(p, m_g, rect, fg);
    }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        QPixmap pm(size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        paint(&p, QRect(QPoint(), size), mode, state);
        return pm;
    }
    QIconEngine *clone() const override { return new GlyphEngine(m_g); }

private:
    Glyph m_g;
};

QIcon glyphIcon(Glyph g) { return QIcon(new GlyphEngine(g)); }

// ---- calendar --------------------------------------------------------------------------------------------

// GtkCalendar: flat grid, dim weekday and week-number labels, accent fill only on the selected day, an accent
// bar under today. Replaces QCalendarView's delegate (only for a plain QCalendarWidget, never a subclass that
// might override paintCell()).
class CalendarDelegate : public QStyledItemDelegate
{
public:
    explicit CalendarDelegate(QCalendarWidget *cw) : QStyledItemDelegate(cw), m_cw(cw) {}

    void paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const AdwaitaTokens *t = tokens();
        if (!t) { QStyledItemDelegate::paint(p, option, index); return; }
        const bool enabled = option.state & QStyle::State_Enabled;
        const bool rowHdr = m_cw->horizontalHeaderFormat() != QCalendarWidget::NoHorizontalHeader;
        const bool colHdr = m_cw->verticalHeaderFormat() != QCalendarWidget::NoVerticalHeader;
        const bool header = (rowHdr && index.row() == 0) || (colHdr && index.column() == 0);
        const QRect cell = option.rect;
        QColor fg = qvariant_cast<QBrush>(index.data(Qt::ForegroundRole)).color();
        if (!fg.isValid() || qvariant_cast<QBrush>(index.data(Qt::ForegroundRole)).style() == Qt::NoBrush)
            fg = option.palette.color(enabled ? QPalette::Active : QPalette::Disabled, QPalette::Text);
        QFont f = option.font;
        bool selected = false, today = false;
        if (header) {
            fg = alphaOf(t->window_fg, 0.55);
            if (index.row() == 0 && rowHdr) { f.setBold(true); if (f.pointSizeF() > 0) f.setPointSizeF(f.pointSizeF() * 0.9); }
        } else {
            const QDate d = dateFor(index, rowHdr, colHdr);
            selected = (option.state & QStyle::State_Selected) && enabled;
            today = d.isValid() && d == QDate::currentDate();
        }
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->fillRect(cell, option.palette.color(QPalette::Base));
        if (!header) {
            const int sz = qMin(qMin(cell.width(), cell.height()) - 6, 40);
            const QRectF pill(cell.center().x() - sz / 2.0, cell.center().y() - sz / 2.0, sz, sz);
            if (selected) {
                p->setPen(Qt::NoPen); p->setBrush(t->accent_bg);
                p->drawRoundedRect(pill, t->radius, t->radius);
                fg = t->accent_fg;
            } else if ((option.state & QStyle::State_MouseOver) && enabled) {
                p->setPen(Qt::NoPen); p->setBrush(alphaOf(t->window_fg, 0.07));
                p->drawRoundedRect(pill, t->radius, t->radius);
            }
            if (today) {  // GTK: inset 0 -2px accent under the day
                p->setPen(Qt::NoPen); p->setBrush(selected ? t->accent_fg : t->accent_color);
                p->drawRoundedRect(QRectF(pill.center().x() - 7, pill.bottom() - 5, 14, 2), 1, 1);
            }
        }
        if (!enabled) fg = alphaOf(fg, 1.0);  // Disabled palette group already dims Text
        p->setFont(f);
        p->setPen(fg);
        p->drawText(cell, Qt::AlignCenter, index.data(Qt::DisplayRole).toString());
        p->restore();
    }

private:
    // The model has no public date lookup: the first cell that shows "1" is the first of the month shown.
    QDate dateFor(const QModelIndex &index, bool rowHdr, bool colHdr) const
    {
        const QAbstractItemModel *m = index.model();
        const int row0 = rowHdr ? 1 : 0, col0 = colHdr ? 1 : 0;
        const QString one = m_cw->locale().toString(1);
        int first = -1;
        for (int i = 0; i < 42 && first < 0; ++i)
            if (m->index(row0 + i / 7, col0 + i % 7).data(Qt::DisplayRole).toString() == one) first = i;
        if (first < 0) return QDate();
        const int pos = (index.row() - row0) * 7 + (index.column() - col0);
        return QDate(m_cw->yearShown(), m_cw->monthShown(), 1).addDays(pos - first);
    }
    QCalendarWidget *m_cw;
};

void polishCalendar(QCalendarWidget *cw)
{
    if (cw->property("adwaita-calendar").toBool()) return;
    cw->setProperty("adwaita-calendar", true);
    if (auto *nav = cw->findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"))) {
        nav->setBackgroundRole(QPalette::Base);  // Qt paints it with the accent colour
        nav->setForegroundRole(QPalette::Text);
    }
    if (auto *b = cw->findChild<QToolButton *>(QStringLiteral("qt_calendar_prevmonth"))) b->setIcon(glyphIcon(GPrev));
    if (auto *b = cw->findChild<QToolButton *>(QStringLiteral("qt_calendar_nextmonth"))) b->setIcon(glyphIcon(GNext));
    // Qt colours Saturday and Sunday red unless the app set its own formats
    for (Qt::DayOfWeek d : {Qt::Saturday, Qt::Sunday})
        if (cw->weekdayTextFormat(d).foreground().color() == QColor(Qt::red)) cw->setWeekdayTextFormat(d, QTextCharFormat());
    if (auto *v = cw->findChild<QTableView *>(QStringLiteral("qt_calendar_calendarview"))) {
        v->setFrameShape(QFrame::NoFrame);
        if (QByteArray(cw->metaObject()->className()) == "QCalendarWidget") v->setItemDelegate(new CalendarDelegate(cw));
    }
}

QPainterPath topRounded(const QRectF &r, qreal rad)
{
    QPainterPath path;
    path.moveTo(r.left(), r.bottom());
    path.lineTo(r.left(), r.top() + rad);
    path.arcTo(QRectF(r.left(), r.top(), 2 * rad, 2 * rad), 180, -90);
    path.lineTo(r.right() - rad, r.top());
    path.arcTo(QRectF(r.right() - 2 * rad, r.top(), 2 * rad, 2 * rad), 90, -90);
    path.lineTo(r.right(), r.bottom());
    path.closeSubpath();
    return path;
}

// ---- MDI title bar ---------------------------------------------------------------------------------------

constexpr int kTitleBtn = 24, kTitleMargin = 6, kTitleGap = 4;

struct TitleButtons { bool close = false, max = false, normalMax = false, min = false, normalMin = false; };

TitleButtons titleButtons(const QStyleOptionTitleBar *tb)
{
    TitleButtons b;
    const auto f = tb->titleBarFlags;
    const auto st = tb->titleBarState;
    const bool maxi = st & Qt::WindowMaximized, mini = st & Qt::WindowMinimized;
    b.close = (tb->subControls & QStyle::SC_TitleBarCloseButton) && (f & Qt::WindowCloseButtonHint);
    b.max = (tb->subControls & QStyle::SC_TitleBarMaxButton) && (f & Qt::WindowMaximizeButtonHint) && !maxi;
    b.normalMax = (tb->subControls & QStyle::SC_TitleBarNormalButton) && (f & Qt::WindowMaximizeButtonHint) && maxi;
    b.min = (tb->subControls & QStyle::SC_TitleBarMinButton) && (f & Qt::WindowMinimizeButtonHint) && !mini;
    b.normalMin = (tb->subControls & QStyle::SC_TitleBarNormalButton) && (f & Qt::WindowMinimizeButtonHint) && mini;
    return b;
}

// Slot 0 is the rightmost: close, then maximize / restore, then minimize.
QRect titleButtonRect(const QStyleOptionTitleBar *tb, QStyle::SubControl sc)
{
    const TitleButtons b = titleButtons(tb);
    int slot = 0, mySlot = -1;
    auto take = [&](bool present, bool mine) { if (!present) return; if (mine) mySlot = slot; ++slot; };
    take(b.close, sc == QStyle::SC_TitleBarCloseButton);
    take(b.max || b.normalMax, sc == QStyle::SC_TitleBarMaxButton ? b.max : sc == QStyle::SC_TitleBarNormalButton ? b.normalMax : false);
    take(b.min || b.normalMin, sc == QStyle::SC_TitleBarMinButton ? b.min : sc == QStyle::SC_TitleBarNormalButton ? b.normalMin && !b.normalMax : false);
    if (mySlot < 0) return QRect();
    const QRect r = tb->rect;
    return QRect(r.right() + 1 - kTitleMargin - kTitleBtn - mySlot * (kTitleBtn + kTitleGap), r.top() + (r.height() - kTitleBtn) / 2, kTitleBtn, kTitleBtn);
}

}  // namespace

// ---- hooks -----------------------------------------------------------------------------------------------

QIcon AdwaitaStyle::standardIcon(StandardPixmap sp, const QStyleOption *opt, const QWidget *w) const
{
    switch (sp) {
    case SP_MessageBoxInformation: return glyphIcon(GInfo);
    case SP_MessageBoxQuestion: return glyphIcon(GQuestion);
    case SP_MessageBoxWarning: return glyphIcon(GWarning);
    case SP_MessageBoxCritical: return glyphIcon(GCritical);
    case SP_DialogOkButton: case SP_DialogApplyButton: case SP_DialogYesButton: return glyphIcon(GCheck);
    case SP_DialogCancelButton: case SP_DialogNoButton: case SP_DialogCloseButton: case SP_DialogDiscardButton:
    case SP_DialogAbortButton: case SP_DialogIgnoreButton: return glyphIcon(GCross);
    case SP_DialogHelpButton: return glyphIcon(GHelp);
    case SP_CommandLink: return QIcon();  // the arrow is drawn at the right edge, like an action row's go-next
    case SP_TitleBarCloseButton: case SP_DockWidgetCloseButton: return glyphIcon(GCross);
    case SP_TitleBarMinButton: return glyphIcon(GMin);
    case SP_TitleBarMaxButton: return glyphIcon(GMax);
    case SP_TitleBarNormalButton: return glyphIcon(GRestore);
    case SP_TitleBarShadeButton: return glyphIcon(GShadeUp);
    case SP_TitleBarUnshadeButton: return glyphIcon(GShadeDown);
    default: break;
    }
    return QCommonStyle::standardIcon(sp, opt, w);
}

bool AdwaitaStyle::widgets_drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (pe) {
    case PE_FrameWindow: {  // MDI sub-window: 1px outline, top corners rounded; the title bar paints over the top
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QRectF r = QRectF(opt->rect).adjusted(.5, .5, -.5, -.5);
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(t.border, 1));
        p->drawPath(topRounded(r, t.radius_popover));
        p->restore();
        return true;
    }
    case PE_FrameDockWidget: {  // floating dock window
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(t.border, 1));
        p->drawRoundedRect(QRectF(opt->rect).adjusted(.5, .5, -.5, -.5), t.radius_card, t.radius_card);
        p->restore();
        return true;
    }
    default: return false;
    }
}

bool AdwaitaStyle::widgets_drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (ce) {
    case CE_ToolBoxTabShape: {  // an expander row: rounded card, hover overlay
        const QRectF r = QRectF(opt->rect).adjusted(0, 2, 0, -2);
        const bool hover = (opt->state & State_MouseOver) && (opt->state & State_Enabled);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(t.card_bg);
        p->drawRoundedRect(r, t.radius, t.radius);
        qreal ov = 0;
        if (opt->state & State_Sunken) ov = 0.08; else if (hover) ov = 0.03; else if (opt->state & State_Selected) ov = 0.04;
        if (ov > 0) { p->setBrush(alphaOf(t.window_fg, ov)); p->drawRoundedRect(r, t.radius, t.radius); }
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(t.border, 1));
        p->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), t.radius, t.radius);
        p->restore();
        return true;
    }
    case CE_ToolBoxTabLabel: {
        auto *tb = qstyleoption_cast<const QStyleOptionToolBox *>(opt);
        if (!tb) return false;
        const bool enabled = opt->state & State_Enabled;
        const bool open = opt->state & State_Selected;
        QRect r = QRect(opt->rect).adjusted(12, 2, -12, -2);
        p->save();
        const QRect arrow(r.right() - 15, r.center().y() - 8, 16, 16);
        r.setRight(arrow.left() - 6);
        if (!tb->icon.isNull()) {
            const QPixmap pm = tb->icon.pixmap(QSize(16, 16), enabled ? QIcon::Normal : QIcon::Disabled);
            p->drawPixmap(QRect(r.left(), r.center().y() - 8, 16, 16), pm);
            r.setLeft(r.left() + 22);
        }
        p->setPen(withDisabled(t.window_fg, opt));
        p->drawText(r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextSingleLine,
                    p->fontMetrics().elidedText(tb->text, Qt::ElideRight, r.width()));
        drawGlyph(p, open ? GShadeUp : GShadeDown, arrow, withDisabled(open ? t.accent_color : t.window_fg, opt));
        p->restore();
        return true;
    }
    case CE_ToolButtonLabel: {  // text under the icon: a 4px gap and both centred in the button
        auto *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
        if (!tb || tb->toolButtonStyle != Qt::ToolButtonTextUnderIcon || tb->text.isEmpty() || tb->icon.isNull()) return false;
        const bool enabled = opt->state & State_Enabled;
        const QIcon::Mode mode = !enabled ? QIcon::Disabled : (opt->state & State_MouseOver) ? QIcon::Active : QIcon::Normal;
        const QSize isz = tb->iconSize;
        const QFontMetrics fm(p->font());
        const int gap = 4, total = isz.height() + gap + fm.height();
        const int top = opt->rect.center().y() - total / 2 + 1;
        const qreal dpr = p->device()->devicePixelRatioF();
        QPixmap pm = tb->icon.pixmap(isz * dpr, mode, (opt->state & State_On) ? QIcon::On : QIcon::Off);
        pm.setDevicePixelRatio(dpr);
        p->drawPixmap(QPoint(opt->rect.center().x() - isz.width() / 2 + 1, top), pm);
        p->setPen(tb->palette.color(enabled ? QPalette::Active : QPalette::Disabled, QPalette::ButtonText));
        p->drawText(QRect(opt->rect.left(), top + isz.height() + gap, opt->rect.width(), fm.height()),
                    Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextSingleLine, tb->text);
        return true;
    }
    case CE_PushButtonBevel: {
        if (!qobject_cast<const QCommandLinkButton *>(w)) return false;
        auto *b = qstyleoption_cast<const QStyleOptionButton *>(opt);
        if (!b) return false;
        // like a row in a boxed list: card, rounded, hover / pressed overlay, go-next chevron at the right edge
        const QRectF r(opt->rect);
        const bool hover = (opt->state & State_MouseOver) && (opt->state & State_Enabled);
        auto body = [&](QPainter *q) {
            q->setRenderHint(QPainter::Antialiasing);
            q->setPen(Qt::NoPen);
            q->setBrush(t.card_bg);
            q->drawRoundedRect(r, t.radius_card, t.radius_card);
            qreal ov = 0;
            if (opt->state & State_Sunken) ov = 0.08; else if (hover) ov = 0.03;
            if (ov > 0) { q->setBrush(alphaOf(t.window_fg, ov)); q->drawRoundedRect(r, t.radius_card, t.radius_card); }
            q->setBrush(Qt::NoBrush);
            q->setPen(QPen(t.border, 1));
            q->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), t.radius_card - .5, t.radius_card - .5);
            drawGlyph(q, GNext, QRectF(r.right() - 12 - 16, r.center().y() - 8, 16, 16), alphaOf(t.window_fg, 0.55));
        };
        p->save();
        if (opt->state & State_Enabled) body(p);
        else drawFaded(p, opt->rect, [&](QPainter *ip) { body(ip); });
        p->restore();
        if (focusVisible(opt, w)) drawFocusRing(p, r, t.radius_card);
        return true;
    }
    default: return false;
    }
}

bool AdwaitaStyle::widgets_drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    if (cc == CC_TitleBar) {
        auto *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt);
        if (!tb) return false;
        const bool active = tb->state & State_Active;
        const qreal dim = active ? 1.0 : 0.5;  // backdrop: content at half opacity
        const QRectF r(tb->rect);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(t.headerbar_bg);
        p->drawPath(topRounded(r, t.radius_popover - 2));  // inside the 2px frame, whose outline is rounded by 15
        p->fillRect(QRect(tb->rect.left() + 1, tb->rect.bottom(), tb->rect.width() - 2, 1), t.headerbar_border);
        // buttons: round, flat, symbolic glyph
        const struct { SubControl sc; Glyph g; bool present; } btns[] = {
            {SC_TitleBarCloseButton, GCross, titleButtons(tb).close},
            {SC_TitleBarMaxButton, GMax, titleButtons(tb).max},
            {SC_TitleBarNormalButton, GRestore, titleButtons(tb).normalMax || titleButtons(tb).normalMin},
            {SC_TitleBarMinButton, GMin, titleButtons(tb).min},
        };
        int leftmost = tb->rect.right() + 1 - kTitleMargin;
        for (const auto &b : btns) {
            if (!b.present) continue;
            const QRect br = titleButtonRect(tb, b.sc);
            if (br.isNull()) continue;
            leftmost = qMin(leftmost, br.left());
            qreal a = 0.10;
            if ((tb->activeSubControls & b.sc) && (tb->state & State_Sunken)) a = 0.30;
            else if ((tb->activeSubControls & b.sc) && (tb->state & State_MouseOver)) a = 0.15;
            p->setBrush(alphaOf(t.headerbar_fg, a * dim));
            p->drawEllipse(QRectF(br).adjusted(0, 0, 0, 0));
            drawGlyph(p, b.g, QRectF(br).adjusted(4, 4, -4, -4), alphaOf(t.headerbar_fg, dim));
        }
        // bold, centred title
        QFont f = p->font();
        f.setBold(true);
        p->setFont(f);
        p->setPen(alphaOf(t.headerbar_fg, dim));
        const int pad = 12;
        QRect tr(tb->rect.left() + pad, tb->rect.top(), qMax(0, leftmost - kTitleMargin - tb->rect.left() - pad), tb->rect.height() - 1);
        const QFontMetrics fm(f);
        const int tw = fm.horizontalAdvance(tb->text);
        QRect centred = QRect(0, tr.top(), qMin(tw, tr.width()), tr.height());
        centred.moveLeft(tb->rect.center().x() - centred.width() / 2);
        if (centred.left() < tr.left()) centred.moveLeft(tr.left());
        if (centred.right() > tr.right()) centred.moveRight(tr.right());
        p->drawText(centred, Qt::AlignCenter | Qt::TextSingleLine, fm.elidedText(tb->text, Qt::ElideRight, tr.width()));
        p->restore();
        return true;
    }
    if (cc == CC_Dial) {
        auto *s = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        if (!s) return false;
        if (!(s->state & State_Enabled)) {
            QStyleOptionSlider en = *s;
            en.state |= State_Enabled;
            en.state &= ~(State_MouseOver | State_Sunken);
            drawFaded(p, s->rect, [&](QPainter *ip) { drawComplexControl(CC_Dial, &en, ip, w); });
            return true;
        }
        const bool hover = s->state & State_MouseOver, active = s->state & State_Sunken;
        const qreal knob = t.scale_slider, th = 4;
        const bool notches = s->subControls & SC_DialTickmarks;
        const QRectF box = QRectF(s->rect);
        const qreal rad = qMax(4.0, qMin(box.width(), box.height()) / 2 - knob / 2 - (notches ? 7 : 1));
        const QPointF c = box.center();
        const QRectF ring(c.x() - rad, c.y() - rad, 2 * rad, 2 * rad);
        const bool wrap = s->dialWrapping;
        const qreal range = s->maximum - s->minimum;
        const qreal frac = range > 0 ? (s->sliderPosition - s->minimum) / range : 0.0;
        const qreal startDeg = wrap ? 270.0 : 240.0, sweep = wrap ? 360.0 : 300.0;
        const qreal angDeg = startDeg - frac * sweep;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setBrush(Qt::NoBrush);
        // trough
        p->setPen(QPen(alphaOf(t.window_fg, (hover ? t.scale_trough_hover_op : t.scale_trough_op)), th, Qt::SolidLine, Qt::RoundCap));
        if (wrap) p->drawEllipse(ring); else p->drawArc(ring, int(startDeg * 16), int(-sweep * 16));
        // value arc
        if (frac > 0) {
            p->setPen(QPen(t.accent_bg, th, Qt::SolidLine, Qt::RoundCap));
            p->drawArc(ring, int(startDeg * 16), int(-frac * sweep * 16));
        }
        // notches
        if (notches && s->tickInterval > 0 && range > 0) {
            p->setPen(QPen(alphaOf(t.window_fg, 0.55), 1));
            const qreal r0 = rad + knob / 2 + 3, r1 = r0 + 3;
            for (int v = s->minimum; v <= s->maximum; v += s->tickInterval) {
                const qreal a = (startDeg - (v - s->minimum) / range * sweep) * M_PI / 180.0;
                p->drawLine(QPointF(c.x() + r0 * std::cos(a), c.y() - r0 * std::sin(a)), QPointF(c.x() + r1 * std::cos(a), c.y() - r1 * std::sin(a)));
            }
        }
        // knob, as on the slider: ring + soft shadow + white-ish disc
        const qreal a = angDeg * M_PI / 180.0;
        const QPointF kc(c.x() + rad * std::cos(a), c.y() - rad * std::sin(a));
        const QRectF kr(kc.x() - knob / 2, kc.y() - knob / 2, knob, knob);
        const bool hc = t.variant.startsWith("hc-");
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(0, 0, 6, 8));
        for (qreal grow : {3.0, 2.2, 1.4, 0.6, -0.2, -1.0}) p->drawEllipse(kr.translated(0, 2).adjusted(-grow, -grow, grow, grow));
        p->setBrush(QColor(0, 0, 6, hc ? 90 : 26));
        p->drawEllipse(kr.adjusted(-1, -1, 1, 1));
        const QColor white(255, 255, 255);
        const QColor idle = QColor::fromRgbF(white.redF() * .8 + t.view_bg.redF() * .2, white.greenF() * .8 + t.view_bg.greenF() * .2, white.blueF() * .8 + t.view_bg.blueF() * .2);
        p->setBrush((hover || active) ? white : idle);
        p->drawEllipse(kr);
        if (focusVisible(opt, w)) {
            p->setBrush(Qt::NoBrush);
            p->setPen(QPen(t.focus_ring, t.focus_ring_width));
            const qreal fw = t.focus_ring_width;
            p->drawEllipse(kr.adjusted(-fw / 2, -fw / 2, fw / 2, fw / 2));
        }
        p->restore();
        return true;
    }
    return false;
}

bool AdwaitaStyle::widgets_subElementRect(SubElement, const QStyleOption *, const QWidget *, QRect &) const { return false; }

bool AdwaitaStyle::widgets_subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *, QRect &out) const
{
    if (cc != CC_TitleBar) return false;
    auto *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt);
    if (!tb) return false;
    switch (sc) {
    case SC_TitleBarCloseButton: case SC_TitleBarMaxButton: case SC_TitleBarMinButton: case SC_TitleBarNormalButton:
        out = titleButtonRect(tb, sc);
        return true;
    case SC_TitleBarSysMenu: case SC_TitleBarShadeButton: case SC_TitleBarUnshadeButton: case SC_TitleBarContextHelpButton:
        out = QRect();
        return true;
    case SC_TitleBarLabel: {
        int left = tb->rect.right() + 1 - kTitleMargin;
        for (SubControl b : {SC_TitleBarCloseButton, SC_TitleBarMaxButton, SC_TitleBarMinButton, SC_TitleBarNormalButton}) {
            const QRect br = titleButtonRect(tb, b);
            if (!br.isNull()) left = qMin(left, br.left() - kTitleGap);
        }
        out = QRect(tb->rect.left() + 12, tb->rect.top(), qMax(0, left - tb->rect.left() - 12), tb->rect.height());
        return true;
    }
    default: return false;
    }
}

bool AdwaitaStyle::widgets_pixelMetric(PixelMetric pm, const QStyleOption *, const QWidget *, int &out) const
{
    switch (pm) {
    case PM_TitleBarHeight: out = 36; return true;
    case PM_MdiSubWindowFrameWidth: out = 2; return true;
    default: return false;
    }
}

bool AdwaitaStyle::widgets_sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &sz, const QWidget *, QSize &out) const
{
    if (ct != CT_ToolButton) return false;
    auto *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
    if (!tb || tb->toolButtonStyle != Qt::ToolButtonTextUnderIcon || tb->text.isEmpty() || tb->icon.isNull()) return false;
    QFont bf = adwLabelFont(tb->font);
    const QFontMetrics fm(bf);
    out = QSize(qMax(fm.horizontalAdvance(tb->text), tb->iconSize.width()) + 24, tb->iconSize.height() + 4 + fm.height() + 16);
    out = out.expandedTo(QSize(sz.width() > 0 ? 56 : 0, 0));
    return true;
}

bool AdwaitaStyle::widgets_styleHint(StyleHint sh, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &out) const
{
    switch (sh) {
    case SH_DialogButtonBox_ButtonsHaveIcons: out = 0; return true;  // GNOME dialog buttons are text only
    default: return false;
    }
}

void AdwaitaStyle::widgets_polish(QWidget *w)
{
    if (auto *cw = qobject_cast<QCalendarWidget *>(w)) polishCalendar(cw);
    if (auto *area = qobject_cast<QMdiArea *>(w)) {
        // the workspace follows the palette instead of Qt's mid-grey (QPalette::Dark)
        const QBrush bg = area->background();
        if (bg.style() == Qt::SolidPattern && bg.color() == area->palette().color(QPalette::Dark))
            area->setBackground(area->palette().brush(QPalette::Window));
    }
    if (auto *tb = qobject_cast<QToolBox *>(w)) {
        const auto all = tb->findChildren<QAbstractButton *>();
        for (QAbstractButton *b : all)
            if (QByteArray(b->metaObject()->className()) == "QToolBoxButton") b->setMinimumHeight(qMax(b->minimumHeight(), 36));
    }
}

void AdwaitaStyle::widgets_unpolish(QWidget *) {}
