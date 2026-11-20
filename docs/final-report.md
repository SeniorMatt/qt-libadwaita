# qt-libadwaita: final report (2026-11-20)

## Versions followed
libadwaita 1.10.0, GNOME 51.1 (checked at the start; no newer release during the work). Default font Adwaita Sans.

## What was built
- `qtlibadwaitastyle.so`: a QStyle plugin, key "Libadwaita", for Qt 6 (built on 6.4, loads in any newer Qt 6) and
  Qt 5.15. Public Qt API only. GPL-2.0-or-later. Repo: ~/qt-adwaita-work/qt-libadwaita.
- Covers buttons (flat, suggested, destructive, pill, circular via the `adwaita-class` property), entries, spin
  buttons, dropdowns, checks, radios, switches, sliders, progress bars, tabs, menus and popovers, tooltips, lists,
  trees, tables, headers, overlay scroll bars, focus rings, header bar and boxed lists (gallery classes).
- Settings from the desktop portal: light/dark, accent colour (nine accents), high contrast, reduced motion; live
  switching. Env overrides: ADWAITA_COLOR_SCHEME, ADWAITA_ACCENT, ADWAITA_HIGH_CONTRAST, ADWAITA_ANIMATIONS.
- User options: ~/.config/qt-libadwaita.conf `spacing=` and `font-size=` (env ADWAITA_SPACING / ADWAITA_FONT_SIZE).
  Default font size 10 pt (owner's choice: Qt/KDE layouts clip text at libadwaita's 11 pt).
- KDE colour schemes (kde-colors/: Libadwaita, Libadwaita Dark, High Contrast, High Contrast Dark), generated from
  the tokens, so KDE apps paint sidebars and views with the same colours.
- Flatpak apps: install.sh adds a user Flatpak override (plugin path read-only); no separate extension package.
- On Plasma, dark and high contrast follow the KDE colour scheme (kdeglobals), live; the portal gives the accent.
- Gallery app, reference shots from real libadwaita, and a comparison tool (tools/compare/run.sh).

## Install and enable
INSTALL.md (any distro) and docs/install-kde-linux.md (immutable KDE Linux). Prebuilt package with installer:
~/qt-adwaita-work/dist/qt-libadwaita-preview.tar.gz (`./install.sh`, then System Settings > Application Style:
Libadwaita and Colours: Libadwaita / Libadwaita Dark).

## Comparison
docs/compare/REPORT.md, sheets in docs/compare/<scheme>-<accent>/. 8 variants (light, dark, hc-light, hc-dark x
blue, orange): 92 OK, 8 partial, 56 fail, 4 no match. Sections fully OK: boxed list, checks, dropdown, entries,
focus rings, header bar, progress, spin button, tabs.

## Remaining differences
- Switch and radio outer edge: 1 px antialiasing/offset differences on the ring (worst with the orange accent).
- Overlay scroll bar: slider colours in dark and high contrast differ (dE up to 20 on the thin bar).
- Menus: 12 px shorter than the reference, which includes the popover arrow gap; Qt menus have no arrow.
- Tooltip size in the reference capture is unreliable (capture anomaly), so it is reported as a fail.
- Without a compositor, menus and tooltips get hard (masked) corners and no shadow; with KWin they are smooth.
- Framed scroll areas (file views, text editors) keep square frames; rounding them broke in third-party apps.
- No backdrop (unfocused window) state. Window title bars come from KWin, not the style.
- Font is 10 pt by default, not libadwaita's 11 (set font-size=11 for the exact size).

## Guidelines checklist
docs/guidelines-checklist.md: 25 ok, 6 partial, 1 todo (backdrop), the rest n/a.

## Third-party apps tested (shots in docs/apps/, light and dark)
- Dolphin 26.08 and Kate 26.08 (Flatpak, KDE runtime 6.11): good with the Libadwaita KDE colour scheme.
- qBittorrent 5.3 (Flatpak): good.
- VLC (host, Qt 5): good, menus included.
- OBS (host, Qt 6): partly; OBS paints most of its window with its own stylesheet theme, only menus and dialogs
  are ours.
- Telegram Desktop: not tested; it bundles Qt statically and ignores style plugins.
- Kinoklip: the change to list the style in its style menu was handed to the Kinoklip session.
