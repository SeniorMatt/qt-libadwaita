// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: scroll bars (libadwaita overlay indicators)
//
// Geometry (docs/development/tokens.md, scrollbar): the QScrollBar widget is 24 px thick (= expanded cross-axis total) and
// overlaps the content (PM_ScrollView_ScrollBarOverlap). Idle it shows a 3 px slider with a 4 px far margin; on
// hover it grows (200 ms) to an 8 px slider, 8 px margin, over a translucent trough. Idle bars fade out after a
// timeout like GTK's overlay scrolling.
#include "adwaitastyle.h"
#include "adwaitasettings.h"

#include <QAbstractScrollArea>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QStyleOptionSlider>
#include <QTimer>
#include <QVariantAnimation>

namespace {

const char *kExpand = "_adw_expand";
const char *kFade = "_adw_fade";
constexpr int kExtent = 24;         // expanded cross-axis total (2*8 + 8)
constexpr int kIdleTimeoutMs = 1000;

bool isOverlayBar(const QWidget *w)
{
    return w && qobject_cast<const QScrollBar *>(w) && qobject_cast<const QAbstractScrollArea *>(w->parentWidget() ? w->parentWidget()->parentWidget() : nullptr);
}

class ScrollAnim : public QObject
{
public:
    explicit ScrollAnim(QScrollBar *b) : QObject(b), m_bar(b)
    {
        setObjectName(QStringLiteral("adwScrollAnim"));  // found by name: the class has no Q_OBJECT
        b->setProperty(kExpand, 0.0);
        b->setProperty(kFade, 1.0);
        m_expand.setDuration(200);
        m_fade.setDuration(200);
        connect(&m_expand, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) { set(kExpand, v.toReal()); });
        connect(&m_fade, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) { set(kFade, v.toReal()); });
        m_idle.setSingleShot(true);
        m_idle.setInterval(kIdleTimeoutMs);
        connect(&m_idle, &QTimer::timeout, this, [this] { if (!m_hover && !m_bar->isSliderDown()) go(m_fade, kFade, 0.0); });
        connect(b, &QScrollBar::valueChanged, this, [this] { flash(); });
        connect(b, &QScrollBar::rangeChanged, this, [this] { flash(); });
        b->installEventFilter(this);
        m_idle.start();  // visible on show, fades after the first timeout
    }
    bool eventFilter(QObject *, QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::Enter: case QEvent::HoverEnter: m_hover = true; go(m_expand, kExpand, 1.0); go(m_fade, kFade, 1.0); m_idle.stop(); break;
        case QEvent::Leave: case QEvent::HoverLeave:
            m_hover = false;
            if (!m_bar->isSliderDown()) go(m_expand, kExpand, 0.0);
            m_idle.start();
            break;
        case QEvent::MouseButtonRelease:
            if (!m_hover) go(m_expand, kExpand, 0.0);
            m_idle.start();
            break;
        default: break;
        }
        return false;
    }

private:
    static bool animated() { return AdwaitaSettings::instance()->animationsEnabled(); }
    void set(const char *name, qreal v) { m_bar->setProperty(name, v); m_bar->update(); }
    void go(QVariantAnimation &a, const char *name, qreal to)
    {
        const qreal from = m_bar->property(name).toReal();
        a.stop();
        if (!animated() || qFuzzyCompare(from + 1, to + 1)) { set(name, to); return; }
        a.setStartValue(from);
        a.setEndValue(to);
        a.start();
    }
    void flash()
    {
        if (!m_bar->isVisible()) return;
        if (m_bar->property(kFade).toReal() < 1.0) go(m_fade, kFade, 1.0);
        m_idle.start();
    }
    QScrollBar *m_bar;
    QVariantAnimation m_expand, m_fade;
    QTimer m_idle;
    bool m_hover = false;
};

}  // namespace

bool AdwaitaStyle::scroll_drawPrimitive(PrimitiveElement pe, const QStyleOption *, QPainter *, const QWidget *) const
{
    return pe == PE_PanelScrollAreaCorner;  // nothing to paint: bars overlay the content
}

bool AdwaitaStyle::scroll_drawControl(ControlElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }

bool AdwaitaStyle::scroll_drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const
{
    if (cc != CC_ScrollBar) return false;
    auto *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt);
    if (!sb) return false;
    const AdwaitaTokens &t = m_tokens;
    if (!(opt->state & State_Enabled)) return true;  // disabled: slider opacity 0
    const bool overlay = isOverlayBar(w);
    const bool horiz = sb->orientation == Qt::Horizontal;
    const bool pressed = (opt->state & State_Sunken) && (opt->activeSubControls & SC_ScrollBarSlider);
    const bool hovered = (opt->state & State_MouseOver) && (opt->activeSubControls & SC_ScrollBarSlider);
    qreal e = overlay ? (w->property(kExpand).isValid() ? w->property(kExpand).toReal() : 0.0) : 1.0;
    qreal fade = overlay && w->property(kFade).isValid() ? w->property(kFade).toReal() : 1.0;
    if (pressed) e = 1.0;
    if (fade <= 0.0) return true;

    const QRect slider = subControlRect(CC_ScrollBar, opt, SC_ScrollBarSlider, w);
    // Work in vertical-bar coordinates; transpose the painter for horizontal bars.
    QRectF box = QRectF(opt->rect), sl = QRectF(slider);
    if (horiz) { box = QRectF(box.y(), box.x(), box.height(), box.width()); sl = QRectF(sl.y(), sl.x(), sl.height(), sl.width()); }
    const bool farLeft = !horiz && opt->direction == Qt::RightToLeft;  // far edge is the one away from the content
    auto lerp = [e](qreal a, qreal b) { return a + (b - a) * e; };
    const qreal th = lerp(t.scrollbar_slider_idle, t.scrollbar_slider_expanded);
    const qreal margin = lerp(t.scrollbar_margin_idle, t.scrollbar_margin_expanded);

    p->save();
    p->setOpacity(fade);
    p->setRenderHint(QPainter::Antialiasing);
    if (horiz) p->setTransform(QTransform(0, 1, 1, 0, 0, 0), true);
    p->setPen(Qt::NoPen);
    const QColor fg = t.window_fg;
    auto fgA = [&](qreal a) { QColor c = fg; c.setAlphaF(c.alphaF() * a); return c; };
    if (overlay && e > 0) {  // translucent trough: an 8px pill exactly under the expanded slider (measured)
        const qreal tx = farLeft ? box.left() + t.scrollbar_margin_expanded : box.right() - t.scrollbar_margin_expanded - t.scrollbar_slider_expanded;
        p->setBrush(fgA(t.scrollbar_trough_op * e));
        p->drawRoundedRect(QRectF(tx, box.top(), t.scrollbar_slider_expanded, box.height()), t.scrollbar_slider_expanded / 2, t.scrollbar_slider_expanded / 2);
    }
    const qreal sx = farLeft ? box.left() + margin : box.right() - margin - th;
    QRectF s(sx, sl.top() + 0, th, sl.height());
    const qreal opa = pressed ? t.scrollbar_op_active : hovered ? t.scrollbar_op_hover : t.scrollbar_op_normal;
    if (overlay) {  // libadwaita: 1px outline around the slider (white on light, black on dark)
        QColor oc = t.variant.endsWith("dark") ? QColor(0, 0, 0) : QColor(255, 255, 255);
        const bool hc = t.variant.startsWith("hc-");
        oc.setAlphaF((hc ? (e > 0.5 ? 1.0 : 0.6) : (e > 0.5 ? 0.6 : 0.35)));
        p->setBrush(oc);
        QPainterPath ring;  // outline only: must not paint under the slider
        ring.addRoundedRect(s.adjusted(-1, -1, 1, 1), th / 2 + 1, th / 2 + 1);
        ring.addRoundedRect(s, th / 2, th / 2);
        ring.setFillRule(Qt::OddEvenFill);
        p->drawPath(ring);
    }
    p->setBrush(fgA(opa));
    p->drawRoundedRect(s, th / 2, th / 2);
    p->restore();
    return true;
}

bool AdwaitaStyle::scroll_subElementRect(SubElement, const QStyleOption *, const QWidget *, QRect &) const { return false; }

bool AdwaitaStyle::scroll_subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *, QRect &out) const
{
    if (cc != CC_ScrollBar) return false;
    auto *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt);
    if (!sb) return false;
    const bool horiz = sb->orientation == Qt::Horizontal;
    const QRect r = opt->rect;
    const int len = horiz ? r.width() : r.height();
    const int minLen = int(m_tokens.scrollbar_slider_min);
    const int range = sb->maximum - sb->minimum;
    int sliderLen = len, sliderPos = 0;
    if (range > 0) {
        sliderLen = qBound(minLen, int(qint64(sb->pageStep) * len / (range + sb->pageStep)), len);
        sliderPos = sliderPositionFromValue(sb->minimum, sb->maximum, sb->sliderPosition, len - sliderLen, sb->upsideDown);
    }
    auto along = [&](int pos, int l) { return horiz ? QRect(r.left() + pos, r.top(), l, r.height()) : QRect(r.left(), r.top() + pos, r.width(), l); };
    switch (sc) {
    case SC_ScrollBarAddLine:
    case SC_ScrollBarSubLine: out = QRect(r.left(), r.top(), 0, 0); return true;
    case SC_ScrollBarGroove: out = r; return true;
    case SC_ScrollBarSlider: out = along(sliderPos, sliderLen); return true;
    case SC_ScrollBarSubPage: out = along(0, sliderPos); return true;
    case SC_ScrollBarAddPage: out = along(sliderPos + sliderLen, len - sliderPos - sliderLen); return true;
    default: break;
    }
    return false;
}

bool AdwaitaStyle::scroll_pixelMetric(PixelMetric pm, const QStyleOption *, const QWidget *w, int &out) const
{
    switch (pm) {
    case PM_ScrollBarExtent: out = kExtent; return true;
    case PM_ScrollBarSliderMin: out = int(m_tokens.scrollbar_slider_min); return true;
    case PM_ScrollView_ScrollBarSpacing: out = 0; return true;
    case PM_ScrollView_ScrollBarOverlap: out = isOverlayBar(w) ? kExtent : 0; return true;
    default: return false;
    }
}

bool AdwaitaStyle::scroll_sizeFromContents(ContentsType, const QStyleOption *, const QSize &, const QWidget *, QSize &) const { return false; }

bool AdwaitaStyle::scroll_styleHint(StyleHint sh, const QStyleOption *, const QWidget *w, QStyleHintReturn *, int &out) const
{
    switch (sh) {
    case SH_ScrollBar_Transient: out = isOverlayBar(w); return true;
    // GTK (gtk-primary-button-warps-slider): primary click on the trough jumps there, middle click pages.
    case SH_ScrollBar_LeftClickAbsolutePosition: out = 1; return true;
    case SH_ScrollBar_MiddleClickAbsolutePosition: out = 0; return true;
    case SH_ScrollBar_ScrollWhenPointerLeavesControl: out = 1; return true;
    default: return false;
    }
}

void AdwaitaStyle::scroll_polish(QWidget *w)
{
    auto *b = qobject_cast<QScrollBar *>(w);
    if (!b) return;
    b->setAttribute(Qt::WA_Hover, true);
    // QScrollBar marks itself opaque at construction unless the style reports it transient, and isOverlayBar() is
    // false then (no parent yet). Our slider is translucent over the content: without this, each repaint (hover,
    // fade) piles alpha on stale pixels and an idle bar turns solid white and never fades away.
    b->setAttribute(Qt::WA_OpaquePaintEvent, false);
    if (!b->findChild<QObject *>(QStringLiteral("adwScrollAnim"), Qt::FindDirectChildrenOnly))
        new ScrollAnim(b);
}

void AdwaitaStyle::scroll_unpolish(QWidget *w)
{
    auto *b = qobject_cast<QScrollBar *>(w);
    if (!b) return;
    delete b->findChild<QObject *>(QStringLiteral("adwScrollAnim"), Qt::FindDirectChildrenOnly);
    b->setAttribute(Qt::WA_Hover, false);
}
