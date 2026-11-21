// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitastyle.h"
#include "adwaitasettings.h"
#include "adwaitaanimations.h"

#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QSettings>
#include <QFileInfo>
#include <QFontInfo>
#include <QComboBox>
#include <QEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QIconEngine>
#include <QPixmapCache>
#include <QPointer>
#include <QStyleOption>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QWidget>

AdwaitaStyle::AdwaitaStyle()
{
    // Adwaita Sans is a variable font. Qt reads the weight of its default instance (400) even for the Bold instance
    // and then emboldens the real bold glyphs again: bold labels came out ~33% heavier than GTK draws them. Turning
    // off Qt's synthetic bold makes them match Pango. It only affects font engines created after this point, so set
    // it as early as the style exists; QT_NO_SYNTHESIZED_BOLD set by the user is kept.
    if (!qEnvironmentVariableIsSet("QT_NO_SYNTHESIZED_BOLD") && !qEnvironmentVariableIsSet("ADWAITA_SYNTHETIC_BOLD"))
        qputenv("QT_NO_SYNTHESIZED_BOLD", "1");
    reload();
    connect(AdwaitaSettings::instance(), &AdwaitaSettings::changed, this, &AdwaitaStyle::settingsChanged);
}

void AdwaitaStyle::reload()
{
    auto *s = AdwaitaSettings::instance();
    m_tokens = AdwaitaTokens::forVariant(s->dark(), s->highContrast(), s->accent());
}

// Apps such as KDE ones set their own palette (KColorScheme, Breeze light) after the style is applied, which mixes
// light and dark. Keep the libadwaita palette unless ADWAITA_KEEP_APP_PALETTE=1. Same for the font: use Adwaita Sans
// 11 when it is installed, unless ADWAITA_KEEP_APP_FONT=1.
// User options: ~/.config/qt-libadwaita.conf ([General] spacing=, font-size=) or env ADWAITA_SPACING / ADWAITA_FONT_SIZE
// (env wins; Flatpak apps: flatpak override --user --env=ADWAITA_SPACING=1.5).
//   spacing   = compact (0.75) | normal (1) | comfortable (1.5) | any number: scales layout gaps, margins, list rows
//   font-size = point size of Adwaita Sans (default 10: Qt/KDE layouts are made for 10 pt and clip text at
//               libadwaita's 11; 11 = exact GNOME size; 0 keeps the app's font)
static QVariant adwOption(const char *env, const char *key)
{
    const QByteArray e = qgetenv(env);
    if (!e.isEmpty()) return QString::fromUtf8(e);
    static QSettings *conf = new QSettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                                               + QStringLiteral("/qt-libadwaita.conf"), QSettings::IniFormat);
    return conf->value(QLatin1String(key));
}
qreal adwSpacingScale()
{
    static const qreal f = [] {
        const QString v = adwOption("ADWAITA_SPACING", "spacing").toString().trimmed().toLower();
        if (v == "compact") return 0.75;
        if (v == "comfortable") return 1.5;
        bool ok = false; const double d = v.toDouble(&ok);
        return (ok && d >= 0.5 && d <= 3) ? d : 1.0;
    }();
    return f;
}
QFont adwLabelFont(QFont f)
{
    static const int weight = [] {  // 0 = keep
        const QString v = adwOption("ADWAITA_BOLD_LABELS", "bold-labels").toString().trimmed().toLower();
        if (v == "0" || v == "false" || v == "no" || v == "off") return 0;
        if (v == "semibold" || v == "600") return 600;
        return 700;
    }();
    if (weight == 0) return f;
    f.setWeight(weight == 600 ? QFont::DemiBold : QFont::Bold);
    f.setStyleStrategy(QFont::StyleStrategy(f.styleStrategy() | QFont::NoSubpixelAntialias));  // GTK labels are grayscale
    return f;
}
static int adwFontSize()
{
    const QVariant v = adwOption("ADWAITA_FONT_SIZE", "font-size");
    bool ok = false; const int n = v.toInt(&ok);
    return (ok && n >= 0 && n <= 40) ? n : 10;
}

static bool s_applyingPalette = false;
static void applyAppPalette(const QPalette &p)
{
    if (qEnvironmentVariableIntValue("ADWAITA_KEEP_APP_PALETTE")) return;
    s_applyingPalette = true;
    QApplication::setPalette(p);
    s_applyingPalette = false;
}

// KDE code that builds a KColorScheme without a config (Dolphin's view background, Kirigami, ...) reads the colour
// scheme file named by the application property KDE_COLOR_SCHEME_PATH, which KColorSchemeManager sets to a Breeze scheme
// when the scheme in kdeglobals cannot be found: a Flatpak app does not see ~/.local/share/color-schemes. A dark libadwaita
// then gets Breeze Light view colours (pure white) under light text. Point the property at our own scheme, written from
// the copy built into the plugin to the user's cache (writable in a sandbox, too).
void AdwaitaStyle::syncKdeColorScheme() const
{
    if (qEnvironmentVariableIntValue("ADWAITA_KEEP_APP_PALETTE") || !qApp) return;
    const QString cur = qApp->property("KDE_COLOR_SCHEME_PATH").toString();
    // Only replace KColorSchemeManager's Breeze fallback: a scheme an app picked on purpose (Kate's scheme menu,
    // Kinoklip's own schemes) is left alone.
    if (cur.isEmpty() || !QFileInfo(cur).fileName().startsWith(QLatin1String("Breeze"))) return;
    const QString v = m_tokens.variant;
    const QString name = QLatin1String("Libadwaita") + (v.startsWith("hc") ? "HighContrast" : "") + (v.endsWith("dark") ? "Dark" : "");
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/qt-libadwaita");
    if (!QDir().mkpath(dir)) return;
    const QString path = dir + QLatin1Char('/') + name + QLatin1String(".colors");
    QFile src(QLatin1String(":/qt-libadwaita/kde-colors/") + name + QLatin1String(".colors"));
    if (!src.open(QIODevice::ReadOnly)) return;
    const QByteArray data = src.readAll();
    QFile cached(path);
    if (!cached.open(QIODevice::ReadOnly) || cached.readAll() != data) {
        cached.close();
        QSaveFile dst(path);  // temp file + rename: other apps may be reading it right now
        if (!dst.open(QIODevice::WriteOnly)) return;
        dst.write(data);
        if (!dst.commit()) return;
    }
    qApp->setProperty("KDE_COLOR_SCHEME_PATH", path);
}

void AdwaitaStyle::polish(QApplication *app)
{
    QCommonStyle::polish(app);
    syncKdeColorScheme();
    if (!qEnvironmentVariableIntValue("ADWAITA_KEEP_APP_FONT") && adwFontSize() > 0) {
        QFont f(QStringLiteral("Adwaita Sans"), adwFontSize());
        if (QFontInfo(f).family() == QLatin1String("Adwaita Sans")) {
            QApplication::setFont(f);
        }
    }
    app->installEventFilter(this);
}

void AdwaitaStyle::unpolish(QApplication *app)
{
    app->removeEventFilter(this);
    QCommonStyle::unpolish(app);
}

// GTK shows mnemonic underlines only while Alt is held (and in menus opened that way). Track Alt application-wide
// and repaint visible windows when it changes; SH_UnderlineShortcut follows it.
static bool s_altHeld = false;
static void setAltHeld(bool on)
{
    if (s_altHeld == on) return;
    s_altHeld = on;
    const auto tops = QApplication::topLevelWidgets();
    for (QWidget *t : tops) {
        if (!t->isVisible()) continue;
        t->update();
        const auto kids = t->findChildren<QWidget *>();
        for (QWidget *k : kids) if (k->isVisible()) k->update();
    }
}

bool AdwaitaStyle::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        auto *k = static_cast<QKeyEvent *>(e);
        if (k->key() == Qt::Key_Alt && !k->isAutoRepeat()) {
            // Released Alt keeps the underlines in a menu opened with Alt+letter until it closes.
            if (e->type() == QEvent::KeyPress) setAltHeld(true);
            else if (!QApplication::activePopupWidget()) setAltHeld(false);
        }
        break;
    }
    case QEvent::Hide:
        if (s_altHeld && o->isWidgetType() && static_cast<QWidget *>(o)->isWindow()
            && static_cast<QWidget *>(o)->windowType() == Qt::Popup
            && !(QGuiApplication::queryKeyboardModifiers() & Qt::AltModifier))
            QMetaObject::invokeMethod(qApp, [] { if (!QApplication::activePopupWidget()) setAltHeld(false); }, Qt::QueuedConnection);
        break;
    case QEvent::Show:
        if (s_altHeld && o->isWidgetType() && static_cast<QWidget *>(o)->windowType() == Qt::Popup
            && !(QGuiApplication::queryKeyboardModifiers() & Qt::AltModifier))
            setAltHeld(false);
        break;
    case QEvent::MouseButtonPress:
    case QEvent::WindowDeactivate:
    case QEvent::ApplicationStateChange:
        if (s_altHeld && !(QGuiApplication::queryKeyboardModifiers() & Qt::AltModifier)) setAltHeld(false);
        break;
    default: break;
    }

    if (o == qApp && e->type() == QEvent::ApplicationPaletteChange && !s_applyingPalette
        && QApplication::palette() != standardPalette()) {
        const QPalette p = standardPalette();
        QMetaObject::invokeMethod(this, [this, p] { applyAppPalette(p); syncKdeColorScheme(); }, Qt::QueuedConnection);
    }
    return QCommonStyle::eventFilter(o, e);
}

void AdwaitaStyle::settingsChanged()
{
    reload();
    if (!qApp) return;
    applyAppPalette(standardPalette());
    syncKdeColorScheme();
    // Re-polish every widget so cached state follows the new variant.
    QList<QPointer<QWidget>> all;
    for (QWidget *w : QApplication::allWidgets()) all.append(w);
    for (const QPointer<QWidget> &w : all) {
        if (!w) continue;
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
    // Selections are a translucent accent tint (list rows 25 %, text 30 %) and libadwaita keeps the view foreground
    // on them, so Highlight is the tint flattened onto the view and HighlightedText stays the normal text colour
    // (custom delegates and text selections that paint HighlightedText on Highlight stay readable).
    QColor sel = t.accent_bg; sel.setAlphaF(0.30);
    p.setColor(QPalette::Highlight, opaque(sel, t.view_bg));
    p.setColor(QPalette::HighlightedText, vfg);
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
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, dis(vfg, t.view_bg));
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
    if (hasClass(w, "suggested-action")) return Suggested;  // classes are space-separated, like CSS
    if (hasClass(w, "destructive-action")) return Destructive;
    if (hasClass(w, "pill")) return Pill;
    if (hasClass(w, "flat")) return Flat;
    if (auto *b = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
        if (b->features & QStyleOptionButton::DefaultButton) return Suggested;
        if (b->features & QStyleOptionButton::Flat) return Flat;
    }
    return Normal;
}

bool AdwaitaStyle::hasClass(const QWidget *w, const char *cls)
{
    return adwClass(w).split(' ', Qt::SkipEmptyParts).contains(QLatin1String(cls));
}

bool AdwaitaStyle::isPillButton(const QWidget *w)
{
    return hasClass(w, "pill");
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

void AdwaitaStyle::drawFocus(QPainter *p, const QRectF &r, qreal radius, bool on, const QWidget *w, quint32 idx) const
{
    const qreal f = w ? AdwAnim::value(w, AdwAnim::key(AdwAnim::Ring, idx), on ? 1.0 : 0.0) : (on ? 1.0 : 0.0);
    if (f > 0) drawFocusRing(p, r, radius, f);
}

void AdwaitaStyle::drawFocusRing(QPainter *p, const QRectF &r, qreal radius, qreal f) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    const qreal w = m_tokens.focus_ring_width * f;  // libadwaita animates outline colour and width
    QColor rc = m_tokens.focus_ring;
    if (f < 1) rc.setAlphaF(rc.alphaF() * f);
    QPen pen(rc, w);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawRoundedRect(r.adjusted(w / 2, w / 2, -w / 2, -w / 2), radius - w / 2, radius - w / 2);
    p->restore();
}

// ---- symbolic icons ---------------------------------------------------------------------------------------

namespace {
// Wraps a "*-symbolic" icon: the source pixmap is only an alpha mask, painted in one colour (disabled: x opacity).
class SymbolicEngine : public QIconEngine
{
public:
    SymbolicEngine(const QIcon &src, const QColor &fg, qreal disabledOpacity) : m_src(src), m_fg(fg), m_dis(disabledOpacity) {}
    QIconEngine *clone() const override { return new SymbolicEngine(m_src, m_fg, m_dis); }
    QString key() const override { return QStringLiteral("adwaita-symbolic"); }
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override { return m_src.actualSize(size, mode, state); }
    // constness of these two differs between Qt 5.15 / 6.x releases: declare both forms, whichever matches overrides
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) const { return m_src.availableSizes(mode, state); }
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) { return m_src.availableSizes(mode, state); }
    QString iconName() const { return m_src.name(); }
    QString iconName() { return m_src.name(); }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override { return scaledPixmap(size, mode, state, 1.0); }
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
    {
        QColor c = m_fg;
        if (mode == QIcon::Disabled) c.setAlphaF(c.alphaF() * m_dis);
        const QString k = QStringLiteral("adwsym-%1-%2-%3x%4-%5-%6").arg(m_src.cacheKey()).arg(c.rgba(), 0, 16).arg(size.width())
                              .arg(size.height()).arg(scale).arg(int(state));
        QPixmap out;
        if (QPixmapCache::find(k, &out)) return out;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QPixmap pm = m_src.pixmap(size, scale, QIcon::Normal, state);
#else
        QPixmap pm = m_src.pixmap(size * scale, QIcon::Normal, state);
        pm.setDevicePixelRatio(scale);
#endif
        if (pm.isNull()) return pm;
        QPainter ip(&pm);  // keep the alpha of the mask, replace the colour
        ip.setCompositionMode(QPainter::CompositionMode_SourceIn);
        ip.fillRect(pm.rect(), c);
        ip.end();
        QPixmapCache::insert(k, pm);
        return pm;
    }
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        const qreal dpr = painter->device()->devicePixelRatioF();
        QPixmap pm = scaledPixmap(rect.size(), mode, state, dpr);
        painter->drawPixmap(rect.topLeft(), pm);
    }
private:
    QIcon m_src;
    QColor m_fg;
    qreal m_dis;
};
}  // namespace

QIcon AdwaitaStyle::symbolicIcon(const QIcon &icon, const QColor &fg) const
{
    if (icon.isNull() || !icon.name().endsWith(QLatin1String("-symbolic"))) return icon;
    return QIcon(new SymbolicEngine(icon, fg, m_tokens.disabled_opacity));
}

// ---- dispatch -------------------------------------------------------------------------------------------

void AdwaitaStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
#define X(F) if (F##_drawPrimitive(pe, opt, p, w)) return;
    ADW_FAMILIES(X)
#undef X
    const AdwaitaTokens &t = m_tokens;
    switch (pe) {
    case PE_PanelButtonCommand:  // KIO breadcrumbs and custom buttons: QCommonStyle paints an opaque square
        if (drawButtonBevel(opt, p, w)) return;
        break;
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {  // pan-*-symbolic: a thin chevron, not Qt's filled triangle
        const QRectF r(opt->rect);
        const qreal s = qBound(2.0, qMin(r.width(), r.height()) / 4.0, 4.0);
        const QPointF m = r.center();
        QPolygonF poly;
        if (pe == PE_IndicatorArrowRight) poly << m + QPointF(-s / 2, -s) << m + QPointF(s / 2, 0) << m + QPointF(-s / 2, s);
        else if (pe == PE_IndicatorArrowLeft) poly << m + QPointF(s / 2, -s) << m + QPointF(-s / 2, 0) << m + QPointF(s / 2, s);
        else if (pe == PE_IndicatorArrowDown) poly << m + QPointF(-s, -s / 2) << m + QPointF(0, s / 2) << m + QPointF(s, -s / 2);
        else poly << m + QPointF(-s, s / 2) << m + QPointF(0, -s / 2) << m + QPointF(s, s / 2);
        QColor c = opt->palette.color(QPalette::ButtonText);
        if (!(opt->state & State_Enabled)) c.setAlphaF(c.alphaF() * 0.55);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(c, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        p->drawPolyline(poly);
        p->restore();
        return;
    }
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
    case PE_FrameStatusBarItem:  // libadwaita has no item frames in a status area
        return;
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
    case CE_ComboBoxLabel: if (drawComboLabel(opt, p, w)) return; break;
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
    case SE_LineEditContents:  // the embedded field of a spin box gets its padding from the spin box itself
        if (w && w->parentWidget() && w->parentWidget()->inherits("QAbstractSpinBox")) return opt->rect;
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
        const bool ed = qstyleoption_cast<const QStyleOptionComboBox *>(opt) && qstyleoption_cast<const QStyleOptionComboBox *>(opt)->editable;
        const int arrowW = 26;  // 16px icon + 10px padding
        const int lp = ed ? 1 : 10;
        switch (sc) {
        case SC_ComboBoxFrame: return r;
        case SC_ComboBoxArrow: return QRect(r.right() - arrowW + 1, r.top(), arrowW, r.height());
        case SC_ComboBoxEditField: return QRect(r.left() + lp, r.top() + 1, r.width() - arrowW - lp - 6, r.height() - 2);
        case SC_ComboBoxListBoxPopup: return r.adjusted(0, 0, 0, 4);  // popover: a small gap below the field
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
    // 1px frames: with Qt's default of 2 the viewport sits one pixel inside the border line and the gap (window
    // colour) reads as a second, double outline around lists, tables and text edits.
    case PM_DefaultFrameWidth: return qobject_cast<const QLineEdit *>(w) ? 0 : 1;
    // Layout gaps follow the libadwaita 6px grid (6 between controls, 12 window margins), times the user's spacing.
    case PM_LayoutHorizontalSpacing:
    case PM_LayoutVerticalSpacing: return qRound(6 * adwSpacingScale());
    case PM_LayoutLeftMargin: case PM_LayoutRightMargin:
    case PM_LayoutTopMargin: case PM_LayoutBottomMargin:
        return qRound(((w && w->isWindow()) ? 12 : 9) * adwSpacingScale());
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
        const bool pill = isPillButton(w);
        const int padX = pill ? 32 : int(t.button_pad_x);  // .pill: padding 10px 32px
        int extra = 0;  // the label is drawn bold
        if (auto *b = qstyleoption_cast<const QStyleOptionButton *>(opt)) if (!b->text.isEmpty()) {
            QFont bf = adwLabelFont(w ? w->font() : QApplication::font());
            extra = QFontMetrics(bf).horizontalAdvance(b->text) - b->fontMetrics.horizontalAdvance(b->text);
        }
        return QSize(qMax(sz.width() + extra + 2 * padX, int(t.button_min_height)),
                     pill ? qMax(sz.height() + 20, 44) : qMax(sz.height() + 10, int(t.button_min_height)));
    }
    case CT_LineEdit:
        return QSize(sz.width() + 2 * int(t.entry_pad_x), qMax(sz.height(), int(t.entry_min_height)));
    case CT_ComboBox: {
        auto *cb = qobject_cast<const QComboBox *>(w);
        if (cb && !cb->isEditable()) {  // GtkDropDown: padding 10, bold label, 6 gap, 16px chevron
            QFont bf = adwLabelFont(cb->font());
            const QFontMetrics fm(bf);
            int tw = 0;
            for (int i = 0; i < cb->count(); ++i) tw = qMax(tw, fm.horizontalAdvance(cb->itemText(i)));
            if (!cb->itemIcon(0).isNull() && cb->count()) tw += cb->iconSize().width() + 6;
            return QSize(tw + 44, qMax(sz.height() + 10, int(t.button_min_height)));
        }
        return QSize(sz.width() + int(t.button_pad_x) + 28, qMax(sz.height() + 10, int(t.button_min_height)));
    }
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
    case SH_UnderlineShortcut: return s_altHeld;
    case SH_Widget_Animate: return AdwaitaSettings::instance()->animationsEnabled();
    case SH_ComboBox_Popup: return 0;
    default: break;
    }
    return QCommonStyle::styleHint(sh, opt, w, ret);
}
