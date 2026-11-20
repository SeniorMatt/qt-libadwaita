// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QColor>
#include <QString>

// Colour and metric tokens per variant. The default values in adwaitatokens.cpp are PLACEHOLDERS
// (approximations from memory); real values come from data/adwaita-1.10.json via tools/gen_tokens.py,
// which writes src/adwaitatokens_data.h (a key/value override table applied on top of the placeholders).
#define ADW_COLOR_TOKENS(X) \
    X(window_bg) X(window_fg) X(view_bg) X(view_fg) X(headerbar_bg) X(headerbar_fg) X(headerbar_border) \
    X(card_bg) X(card_fg) X(popover_bg) X(popover_fg) X(dialog_bg) \
    X(button_bg) X(button_hover_bg) X(button_active_bg) X(entry_bg) X(border) X(control_bg) X(control_border) \
    X(accent_bg) X(accent_fg) X(accent_color) X(destructive_bg) X(destructive_fg) X(destructive_color) \
    X(success_color) X(warning_color) X(error_color) X(shade) X(tooltip_bg) X(tooltip_fg) X(focus_ring)

#define ADW_METRIC_TOKENS(X) \
    X(radius) X(radius_card) X(button_min_height) X(button_pad_x) X(entry_pad_x) X(entry_min_height) \
    X(check_size) X(switch_width) X(switch_height) X(border_width) X(focus_ring_width) \
    X(disabled_opacity) X(spacing) X(border_opacity) X(focus_ring_alpha) X(radius_popover) X(radius_check) X(radius_switch)

struct AdwaitaTokens {
#define X(n) QColor n;
    ADW_COLOR_TOKENS(X)
#undef X
#define X(n) double n = 0;
    ADW_METRIC_TOKENS(X)
#undef X
    QString variant;

    // variant is "light", "dark", "hc-light" or "hc-dark"; accent = accent background colour.
    static AdwaitaTokens forVariant(bool dark, bool highContrast, const QColor &accent);
    bool set(const QString &key, const QString &value);  // used by the generated data table
};
