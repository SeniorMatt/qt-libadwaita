// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitatokens.h"

#include <QRegularExpression>

struct AdwaitaTokenOverride { const char *variant; const char *key; const char *value; };
struct AdwaitaAccentEntry { const char *name; unsigned bg; const char *light; const char *dark; };
#include "adwaitatokens_data.h"  // defines kAdwaitaTokenOverrides[] / kAdwaitaTokenOverrideCount

static QColor parseColor(const QString &s)
{
    static const QRegularExpression rgba(R"(rgba?\(\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*(?:,\s*([\d.]+)\s*)?\))");
    auto m = rgba.match(s);
    if (m.hasMatch()) {
        QColor c(qRound(m.captured(1).toDouble()), qRound(m.captured(2).toDouble()), qRound(m.captured(3).toDouble()));
        if (!m.captured(4).isEmpty()) c.setAlphaF(m.captured(4).toDouble());
        return c;
    }
    if (s.size() == 9 && s.startsWith('#'))  // #rrggbbaa (CSS order), QColor wants #aarrggbb
        return QColor(QStringLiteral("#") + s.mid(7, 2) + s.mid(1, 6));
    return QColor(s);
}

bool AdwaitaTokens::set(const QString &key, const QString &value)
{
#define X(n) if (key == QLatin1String(#n)) { QColor c = parseColor(value); if (c.isValid()) n = c; return c.isValid(); }
    ADW_COLOR_TOKENS(X)
#undef X
#define X(n) if (key == QLatin1String(#n)) { bool ok; double v = value.toDouble(&ok); if (ok) n = v; return ok; }
    ADW_METRIC_TOKENS(X)
#undef X
    return false;
}

static QColor withAlpha(QColor c, qreal a) { c.setAlphaF(c.alphaF() * a); return c; }

AdwaitaTokens AdwaitaTokens::forVariant(bool dark, bool hc, const QColor &accent)
{
    AdwaitaTokens t;
    t.variant = QString(hc ? "hc-" : "") + (dark ? "dark" : "light");
    // ---- PLACEHOLDER base values; real ones come from the generated override table below ----------------
    if (!dark) {
        t.window_bg = "#fafafb"; t.window_fg = QColor(0, 0, 6, 0xcc);
        t.view_bg = "#ffffff"; t.view_fg = t.window_fg;
        t.headerbar_bg = "#ffffff"; t.headerbar_fg = t.window_fg; t.headerbar_border = t.window_fg;
        t.card_bg = "#ffffff"; t.card_fg = t.window_fg;
        t.popover_bg = "#ffffff"; t.popover_fg = t.window_fg; t.dialog_bg = "#fafafb";
        t.shade = QColor(0, 0, 6, 0x12);
    } else {
        t.window_bg = "#222226"; t.window_fg = "#ffffff";
        t.view_bg = "#1d1d20"; t.view_fg = "#ffffff";
        t.headerbar_bg = "#2e2e32"; t.headerbar_fg = "#ffffff"; t.headerbar_border = "#ffffff";
        t.card_bg = QColor(255, 255, 255, 0x14); t.card_fg = "#ffffff";
        t.popover_bg = "#36363a"; t.popover_fg = "#ffffff"; t.dialog_bg = "#36363a";
        t.shade = QColor(0, 0, 0, 0x4d);
    }
    t.tooltip_bg = QColor(0, 0, 6, 0xcc); t.tooltip_fg = "#ffffff";
    t.destructive_bg = dark ? "#c01c28" : "#e01b24"; t.destructive_fg = "#ffffff";
    t.border_opacity = hc ? 0.5 : 0.15; t.disabled_opacity = hc ? 0.4 : 0.5; t.focus_ring_alpha = hc ? 0.8 : 0.5;
    t.radius = 9; t.radius_card = 12; t.radius_popover = 15; t.radius_check = 6; t.radius_switch = 14;
    t.button_min_height = 34; t.button_pad_x = 17; t.entry_pad_x = 9; t.entry_min_height = 34;
    t.check_size = 20; t.switch_width = 46; t.switch_height = 26; t.border_width = 1; t.focus_ring_width = 2; t.spacing = 6;
    // ---- generated overrides (base colours, radii, opacities) -------------------------------------------
    for (int i = 0; i < kAdwaitaTokenOverrideCount; ++i)
        if (t.variant == QLatin1String(kAdwaitaTokenOverrides[i].variant))
            t.set(QLatin1String(kAdwaitaTokenOverrides[i].key), QLatin1String(kAdwaitaTokenOverrides[i].value));
    // ---- derived colours (currentColor-based in libadwaita) ----------------------------------------------
    const QColor fg = t.window_fg;
    t.button_bg = withAlpha(fg, 0.10); t.button_hover_bg = withAlpha(fg, 0.15); t.button_active_bg = withAlpha(fg, 0.30);
    t.entry_bg = withAlpha(fg, 0.10);
    t.border = withAlpha(fg, t.border_opacity); t.control_border = t.border;
    t.control_bg = withAlpha(fg, hc ? 0.30 : 0.15);  // switch trough / unchecked check
    t.headerbar_border = withAlpha(t.headerbar_fg, t.border_opacity);
    t.accent_bg = accent; t.accent_fg = "#ffffff";
    // standalone accent (text/ring colour): table from JSON for named accents, rough fallback otherwise
    bool found = false;
    for (int i = 0; i < kAdwaitaAccentCount; ++i)
        if (kAdwaitaAccents[i].bg == (unsigned)(accent.rgb() & 0xffffff)) {
            t.accent_color = QColor(QLatin1String(dark ? kAdwaitaAccents[i].dark : kAdwaitaAccents[i].light)); found = true;
        }
    if (!found) t.accent_color = dark ? accent.lighter(135) : accent.darker(115);
    t.destructive_color = dark ? "#ff938c" : "#c30000";
    t.success_color = dark ? "#78e9ab" : "#007c3d"; t.warning_color = dark ? "#ffc252" : "#905400"; t.error_color = t.destructive_color;
    t.focus_ring = withAlpha(t.accent_color, t.focus_ring_alpha);
    return t;
}
