// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: sliders, progress bars, spin boxes (libadwaita 1.10 look)
#include "adwaitastyle.h"
#include "adwaitaanimations.h"
#include "adwaitasettings.h"

#include <QAbstractSpinBox>
#include <QElapsedTimer>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QSlider>
#include <QStyleOptionProgressBar>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QTimer>
#include <cmath>

namespace {

constexpr int kSpinBtnW = 33;       // measured in libadwaita 1.10 shots: +/- cell width incl. 1px separator
constexpr int kSpinTextPad = 6;     // text min width 28 + padding 6
constexpr int kTrough = 4;          // measured: scale trough is 4px thick (SCSS token says 10, pixels win)
constexpr int kTickLen = 6, kTickDist = 6;
constexpr int kProgressTextGap = 6;

// libadwaita progress bars are a bare thin bar: widget bars (QProgressBar) drop the percentage text. Bars painted for
// item-view cells (delegates pass the view, not a QProgressBar) keep it, since there it is the only readout;
// an app can ask for it on a widget with the property adwaita-progress-text = true.
bool showProgressText(const QStyleOptionProgressBar *pb, const QWidget *w)
{
    if (!pb || !pb->textVisible) return false;
    if (!qobject_cast<const QProgressBar *>(w)) return true;
    return w->property("adwaita-progress-text").toBool();
}

QColor mixOver(const QColor &top, const QColor &bottom, qreal a)  // top at alpha a over bottom
{
    return QColor::fromRgbF(top.redF() * a + bottom.redF() * (1 - a), top.greenF() * a + bottom.greenF() * (1 - a),
                            top.blueF() * a + bottom.blueF() * (1 - a));
}

QColor alphaOf(QColor c, qreal a) { c.setAlphaF(c.alphaF() * a); return c; }

}  // namespace

// ---- slider geometry -------------------------------------------------------------------------------------

static void sliderRects(const AdwaitaTokens &t, const QStyleOptionSlider *s, QRect &groove, QRect &handle)
{
    const bool horiz = s->orientation == Qt::Horizontal;
    const int cell = int(t.scale_trough + 2 * t.scale_pad);  // 34
    const int knob = int(t.scale_slider);
    const int tickSpace = int(kTickLen + kTickDist);
    const QRect r = s->rect;
    // control cell (the part holding groove+knob), tick marks live outside of it
    int crossStart = horiz ? r.top() : r.left();
    if (s->tickPosition & QSlider::TicksAbove) crossStart += tickSpace;
    const int center = crossStart + cell / 2;
    const int len = horiz ? r.width() : r.height();
    const int th = kTrough;
    if (horiz) groove = QRect(r.left(), center - th / 2, len, th);
    else groove = QRect(center - th / 2, r.top(), th, len);
    const int span = len - knob;
    const int pos = QStyle::sliderPositionFromValue(s->minimum, s->maximum, s->sliderPosition, span, s->upsideDown);
    if (horiz) handle = QRect(r.left() + pos, center - knob / 2, knob, knob);
    else handle = QRect(center - knob / 2, r.top() + (s->upsideDown ? pos : span - pos), knob, knob);
}

bool AdwaitaStyle::range_drawPrimitive(PrimitiveElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }

bool AdwaitaStyle::range_drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
    if (ce != CE_ProgressBar) return false;
    auto *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt);
    if (!pb) return false;
    const AdwaitaTokens &t = m_tokens;
    const bool showText = showProgressText(pb, w);
    const bool horiz = bool(pb->state & State_Horizontal);
    const int thick = int(t.progress_thickness);
    const bool hc = t.variant.startsWith("hc-");
    QRectF bar;
    QRect textRect;
    if (horiz) {
        int h = showText ? thick + kProgressTextGap + pb->fontMetrics.height() : thick;
        QRect r = opt->rect;
        const int top = r.top() + (r.height() - h) / 2;
        bar = QRectF(r.left(), top + h - thick, r.width(), thick);
        if (showText) textRect = QRect(r.left(), top, r.width(), h - thick - kProgressTextGap);
    } else {
        bar = QRectF(opt->rect.left() + (opt->rect.width() - thick) / 2, opt->rect.top(), thick, opt->rect.height());
    }
    const qreal rad = thick / 2.0;
    const qreal len = horiz ? bar.width() : bar.height();
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    const qreal dis = (opt->state & State_Enabled) ? 1.0 : t.disabled_opacity;
    p->setOpacity(dis);
    p->setBrush(alphaOf(t.window_fg, hc ? 0.30 : t.scale_trough_op));
    p->drawRoundedRect(bar, rad, rad);
    if (hc) {
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(t.border, 1));
        p->drawRoundedRect(bar.adjusted(.5, .5, -.5, -.5), rad, rad);
        p->setPen(Qt::NoPen);
    }
    // fill, clipped to the trough so its ends stay round
    QPainterPath clip;
    clip.addRoundedRect(bar, rad, rad);
    p->setClipPath(clip);
    p->setBrush(t.accent_bg);
    const bool inv = pb->invertedAppearance;
    const bool indet = pb->minimum == 0 && pb->maximum == 0;
    qreal from = 0, size = 0;
    if (indet) {  // pulsing pill sliding back and forth (AdwProgressBar pulse)
        static QElapsedTimer clock;
        if (!clock.isValid()) clock.start();
        size = len * 0.3;
        const qreal ph = std::fmod(clock.elapsed() / 2000.0, 1.0);
        const qreal tri = ph < 0.5 ? ph * 2 : (1 - ph) * 2;
        from = (len - size) * tri;
    } else if (pb->maximum > pb->minimum && pb->progress > pb->minimum) {
        const qreal f = qreal(pb->progress - pb->minimum) / (pb->maximum - pb->minimum);
        size = qMax<qreal>(thick, len * f);  // fill is never narrower than the trough thickness
        from = 0;
    }
    if (size > 0) {
        QRectF fill;
        if (horiz) {
            const bool rtl = inv != (opt->direction == Qt::RightToLeft);
            fill = QRectF(rtl ? bar.right() - from - size : bar.left() + from, bar.top(), size, thick);
        } else {
            const bool top = (pb->bottomToTop ? !inv : inv) ? false : true;  // default fills from the bottom for bottomToTop
            fill = QRectF(bar.left(), (pb->bottomToTop != inv) ? bar.bottom() - from - size : bar.top() + from, thick, size);
            Q_UNUSED(top)
        }
        p->drawRoundedRect(fill, rad, rad);
    }
    p->restore();
    if (showText && horiz && !pb->text.isEmpty()) {  // smaller, dimmed text above the bar
        p->save();
        QFont f = p->font();
        f.setPointSizeF(f.pointSizeF() * 0.83);
        p->setFont(f);
        p->setPen(alphaOf(t.window_fg, hc ? 0.9 : 0.55));
        p->setOpacity(dis);
        p->drawText(textRect, Qt::AlignCenter | Qt::AlignBottom, pb->text);
        p->restore();
    }
    return true;
}

bool AdwaitaStyle::range_drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const
{
    const AdwaitaTokens &t = m_tokens;
    const bool hc = t.variant.startsWith("hc-");
    if (cc == CC_Slider) {
        auto *s = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        if (!s) return false;
        if (!(s->state & State_Enabled)) {  // whole widget faded as one group
            QStyleOptionSlider en = *s;
            en.state |= State_Enabled;
            en.state &= ~(State_MouseOver | State_Sunken);
            drawFaded(p, s->rect, [&](QPainter *ip) { drawComplexControl(CC_Slider, &en, ip, w); });
            return true;
        }
        const bool horiz = s->orientation == Qt::Horizontal;
        const bool enabled = true;
        const bool hover = (s->state & State_MouseOver) && (s->activeSubControls & SC_SliderHandle);
        const bool grooveHover = (s->state & State_MouseOver);
        const bool active = (s->state & State_Sunken) && (s->activeSubControls & SC_SliderHandle);
        QRect g, h;
        sliderRects(t, s, g, h);
        const qreal rad = kTrough / 2.0;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setOpacity(enabled ? 1.0 : t.disabled_opacity);
        // trough
        p->setBrush(AdwAnim::color(w, AdwAnim::key(AdwAnim::Trough), alphaOf(t.window_fg, grooveHover ? t.scale_trough_hover_op : t.scale_trough_op)));
        p->drawRoundedRect(QRectF(g), rad, rad);
        if (hc) {
            p->setBrush(Qt::NoBrush);
            p->setPen(QPen(t.border, 1));
            p->drawRoundedRect(QRectF(g).adjusted(.5, .5, -.5, -.5), rad, rad);
            p->setPen(Qt::NoPen);
        }
        // highlight: from the origin to the knob centre
        QRectF fill = QRectF(g);
        const QPointF hc2 = QRectF(h).center();
        const bool fromEnd = horiz ? (s->upsideDown != (opt->direction == Qt::RightToLeft && false)) : !s->upsideDown;
        if (horiz) { if (fromEnd) fill.setLeft(hc2.x()); else fill.setRight(hc2.x()); }
        else { if (fromEnd && s->upsideDown) fill.setBottom(hc2.y()); else if (fromEnd) fill.setTop(hc2.y()); else fill.setBottom(hc2.y()); }
        if (!horiz && !s->upsideDown) { fill = QRectF(g); fill.setTop(hc2.y()); }
        if (!horiz && s->upsideDown) { fill = QRectF(g); fill.setBottom(hc2.y()); }
        QPainterPath clip;
        clip.addRoundedRect(QRectF(g), rad, rad);
        p->save();
        p->setClipPath(clip);
        p->setBrush(t.accent_bg);
        p->drawRect(fill);
        const QColor hl = AdwAnim::color(w, AdwAnim::key(AdwAnim::Fill), (hover || active) ? alphaOf(t.window_fg, 0.10) : QColor(Qt::transparent));
        if (hl.alpha() > 0) { p->setBrush(hl); p->drawRect(fill); }
        p->restore();
        // tick marks
        if (s->tickPosition != QSlider::NoTicks && s->tickInterval > 0) {
            const qreal dim = hc ? 0.9 : 0.55;
            p->setPen(QPen(alphaOf(t.window_fg, dim), 1));
            const int span = (horiz ? s->rect.width() : s->rect.height()) - int(t.scale_slider);
            for (int v = s->minimum; v <= s->maximum; v += s->tickInterval) {
                int pos = sliderPositionFromValue(s->minimum, s->maximum, v, span, s->upsideDown) + int(t.scale_slider) / 2;
                if (!horiz) pos = (s->rect.height() - int(t.scale_slider)) - (pos - int(t.scale_slider) / 2) + int(t.scale_slider) / 2;
                auto line = [&](qreal edge, int dir) {
                    if (horiz) p->drawLine(QPointF(s->rect.left() + pos + .5, edge), QPointF(s->rect.left() + pos + .5, edge + dir * kTickLen));
                    else p->drawLine(QPointF(edge, s->rect.top() + pos + .5), QPointF(edge + dir * kTickLen, s->rect.top() + pos + .5));
                };
                const qreal below = horiz ? g.bottom() + 1 + kTickDist : g.right() + 1 + kTickDist;
                const qreal above = horiz ? g.top() - kTickDist : g.left() - kTickDist;
                if (s->tickPosition & QSlider::TicksBelow) line(below, 1);
                if (s->tickPosition & QSlider::TicksAbove) line(above, -1);
            }
            p->setPen(Qt::NoPen);
        }
        // knob: shadow (0 2px 4px rgba(0,0,6,.2), ring 1px rgba(0,0,6,.1 / hc .35)), white fill
        const QRectF kr(h);
        if (enabled) {
            p->setBrush(QColor(0, 0, 6, 8));
            for (qreal grow : {3.0, 2.2, 1.4, 0.6, -0.2, -1.0})  // cheap blur: stacked translucent discs
                p->drawEllipse(kr.translated(0, 2).adjusted(-grow, -grow, grow, grow));
            p->setBrush(QColor(0, 0, 6, hc ? 90 : 26));
            p->drawEllipse(kr.adjusted(-1, -1, 1, 1));
        }
        const QColor white(255, 255, 255);
        p->setBrush(AdwAnim::color(w, AdwAnim::key(AdwAnim::Knob), (hover || active) ? white : mixOver(white, t.view_bg, 0.8)));
        p->drawEllipse(kr);
        p->restore();
        if (focusVisible(opt, w)) {
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setBrush(Qt::NoBrush);
            p->setPen(QPen(t.focus_ring, t.focus_ring_width));
            const qreal fw = t.focus_ring_width;
            p->drawEllipse(kr.adjusted(-fw / 2, -fw / 2, fw / 2, fw / 2));
            p->restore();
        }
        return true;
    }
    if (cc == CC_SpinBox) {
        auto *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt);
        if (!sb) return false;
        const QRectF r(opt->rect);
        const bool enabled = opt->state & State_Enabled;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setOpacity(enabled ? 1.0 : t.disabled_opacity);
        p->setBrush(t.entry_bg);
        p->drawRoundedRect(r, t.radius, t.radius);
        if (hc) {
            p->setBrush(Qt::NoBrush);
            p->setPen(QPen(t.border, 1));
            p->drawRoundedRect(r.adjusted(.5, .5, -.5, -.5), t.radius, t.radius);
            p->setPen(Qt::NoPen);
        }
        if (sb->buttonSymbols != QAbstractSpinBox::NoButtons) {
            const QRectF down(subControlRect(CC_SpinBox, opt, SC_SpinBoxDown, w));
            const QRectF up(subControlRect(CC_SpinBox, opt, SC_SpinBoxUp, w));
            struct B { QRectF r; bool isUp; };
            for (const B &b : {B{down, false}, B{up, true}}) {
                const SubControl sc = b.isUp ? SC_SpinBoxUp : SC_SpinBoxDown;
                const bool on = (sb->stepEnabled & (b.isUp ? QAbstractSpinBox::StepUpEnabled : QAbstractSpinBox::StepDownEnabled)) != 0;
                const bool hov = enabled && (opt->state & State_MouseOver) && (opt->activeSubControls & sc);
                const bool prs = enabled && (opt->state & State_Sunken) && (opt->activeSubControls & sc);
                const QColor ov = AdwAnim::color(w, AdwAnim::key(AdwAnim::Fill, b.isUp ? 1 : 2),
                                                 (hov || prs) ? alphaOf(t.window_fg, prs ? 0.16 : 0.07) : QColor(Qt::transparent));
                if (ov.alpha() > 0) {  // hover/active overlay (200 ms), outer corners keep the entry radius
                    QPainterPath path;
                    path.addRoundedRect(r, t.radius, t.radius);
                    QPainterPath cell;
                    cell.addRect(b.r);
                    p->setBrush(ov);
                    p->drawPath(path.intersected(cell));
                }
                // 1px separator on the inner (left) edge
                p->setBrush(alphaOf(t.window_fg, hc ? 0.5 : 0.10));
                p->drawRect(QRectF(b.r.left(), b.r.top(), 1, b.r.height()));
                // symbolic minus / plus glyph
                const QPointF c = b.r.center();
                QColor gc = alphaOf(t.window_fg, on ? 1.0 : t.disabled_opacity);
                p->setPen(QPen(gc, 1.6, Qt::SolidLine, Qt::RoundCap));
                const qreal a = 4.0;
                p->drawLine(QPointF(c.x() - a, c.y()), QPointF(c.x() + a, c.y()));
                if (b.isUp) p->drawLine(QPointF(c.x(), c.y() - a), QPointF(c.x(), c.y() + a));
                p->setPen(Qt::NoPen);
            }
        }
        p->restore();
        drawFocus(p, r, t.radius, opt->state & State_HasFocus, w);
        return true;
    }
    return false;
}

bool AdwaitaStyle::range_subElementRect(SubElement se, const QStyleOption *opt, const QWidget *, QRect &out) const
{
    switch (se) {
    case SE_ProgressBarGroove:
    case SE_ProgressBarContents:
    case SE_ProgressBarLabel: out = opt->rect; return true;
    default: return false;
    }
}

bool AdwaitaStyle::range_subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *, QRect &out) const
{
    const AdwaitaTokens &t = m_tokens;
    if (cc == CC_Slider) {
        auto *s = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        if (!s) return false;
        QRect g, h;
        sliderRects(t, s, g, h);
        switch (sc) {
        case SC_SliderGroove: out = g; return true;
        case SC_SliderHandle: out = h; return true;
        case SC_SliderTickmarks: out = s->rect; return true;
        default: return false;
        }
    }
    if (cc == CC_SpinBox) {
        auto *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt);
        if (!sb) return false;
        const QRect r = opt->rect;
        const int bw = sb->buttonSymbols == QAbstractSpinBox::NoButtons ? 0 : kSpinBtnW;
        const bool rtl = opt->direction == Qt::RightToLeft;
        // visual order (LTR): [ edit | minus | plus ]
        auto cell = [&](int idx) {  // idx 0 = plus (outermost), 1 = minus
            return rtl ? QRect(r.left() + idx * bw, r.top(), bw, r.height()) : QRect(r.right() + 1 - (idx + 1) * bw, r.top(), bw, r.height());
        };
        switch (sc) {
        case SC_SpinBoxFrame: out = r; return true;
        case SC_SpinBoxUp: out = bw ? cell(0) : QRect(); return true;
        case SC_SpinBoxDown: out = bw ? cell(1) : QRect(); return true;
        case SC_SpinBoxEditField: {
            const int pad = kSpinTextPad;
            out = rtl ? QRect(r.left() + 2 * bw + pad, r.top(), r.width() - 2 * bw - 2 * pad, r.height())
                      : QRect(r.left() + pad, r.top(), r.width() - 2 * bw - 2 * pad, r.height());
            return true;
        }
        default: return false;
        }
    }
    return false;
}

bool AdwaitaStyle::range_pixelMetric(PixelMetric pm, const QStyleOption *, const QWidget *, int &out) const
{
    const AdwaitaTokens &t = m_tokens;
    switch (pm) {
    case PM_SliderThickness: out = int(t.scale_trough + 2 * t.scale_pad); return true;
    case PM_SliderControlThickness: out = int(t.scale_slider); return true;
    case PM_SliderLength: out = int(t.scale_slider); return true;
    case PM_SliderTickmarkOffset: out = kTickLen + kTickDist; return true;
    case PM_SliderSpaceAvailable: return false;
    case PM_ProgressBarChunkWidth: out = 1; return true;
    case PM_SpinBoxFrameWidth: out = 0; return true;
    default: return false;
    }
}

bool AdwaitaStyle::range_sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &sz, const QWidget *w, QSize &out) const
{
    const AdwaitaTokens &t = m_tokens;
    if (ct == CT_ProgressBar) {
        auto *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt);
        const bool horiz = !pb || (pb->state & State_Horizontal);
        const int thick = int(t.progress_thickness);
        if (horiz) {
            int h = thick;
            if (showProgressText(pb, w)) h += kProgressTextGap + sz.height();
            out = QSize(qMax(int(t.progress_min_length), sz.width()), h);
        } else {
            out = QSize(thick, 80);  // vertical min length (progressbar.min_length_v)
        }
        return true;
    }
    if (ct == CT_SpinBox) {
        auto *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt);
        const int bw = (sb && sb->buttonSymbols == QAbstractSpinBox::NoButtons) ? 0 : kSpinBtnW;
        out = QSize(qMax(sz.width(), 28) + 2 * bw + 2 * kSpinTextPad, qMax(sz.height(), int(t.entry_min_height)));
        return true;
    }
    return false;
}

bool AdwaitaStyle::range_styleHint(StyleHint sh, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &out) const
{
    switch (sh) {
    // GTK scales: primary click warps the knob to the pointer, middle click pages.
    case SH_Slider_AbsoluteSetButtons: out = Qt::LeftButton; return true;
    case SH_Slider_PageSetButtons: out = Qt::MiddleButton; return true;
    case SH_Slider_SnapToValue: out = 0; return true;
    case SH_SpinBox_ClickAutoRepeatRate: out = 100; return true;
    case SH_SpinBox_ButtonsInsideFrame: return false;
    default: return false;
    }
}

void AdwaitaStyle::range_polish(QWidget *w)
{
    if (qobject_cast<QSlider *>(w) || qobject_cast<QAbstractSpinBox *>(w))
        w->setAttribute(Qt::WA_Hover, true);
    if (auto *pb = qobject_cast<QProgressBar *>(w)) {  // repaint driver for the indeterminate pulse
        if (pb->findChild<QTimer *>("adw-pulse")) return;
        auto *tm = new QTimer(pb);
        tm->setObjectName("adw-pulse");
        tm->setInterval(33);
        QObject::connect(tm, &QTimer::timeout, pb, [pb] {
            if (pb->minimum() == 0 && pb->maximum() == 0 && pb->isVisible()) pb->update();
        });
        tm->start();
    }
}

void AdwaitaStyle::range_unpolish(QWidget *w)
{
    if (qobject_cast<QSlider *>(w) || qobject_cast<QAbstractSpinBox *>(w))
        w->setAttribute(Qt::WA_Hover, false);
    if (auto *pb = qobject_cast<QProgressBar *>(w))
        delete pb->findChild<QTimer *>("adw-pulse");
}
