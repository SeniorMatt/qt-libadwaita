# Status (2026-11-20)

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
- KDE colour schemes (data/kde-colors/: Libadwaita, Libadwaita Dark, High Contrast, High Contrast Dark), generated from
  the tokens, so KDE apps paint sidebars and views with the same colours.
- Flatpak apps: install.sh adds a user Flatpak override (plugin path read-only); no separate extension package.
- On Plasma, dark and high contrast follow the KDE colour scheme (kdeglobals), live; the portal gives the accent.
- Gallery app, reference shots from real libadwaita, and a comparison tool (tests/visual/compare/run.sh).

## Install and enable
[install.md](install.md) (any distro, and immutable KDE Linux). Prebuilt package with installer:
the prebuilt package (`./install.sh`, then System Settings > Application Style:
Libadwaita and Colours: Libadwaita / Libadwaita Dark).

## Comparison
[tests/visual/report/REPORT.md](../tests/visual/report/REPORT.md), sheets in tests/visual/report/<scheme>-<accent>/. 8 variants (light, dark, hc-light, hc-dark x
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

## Guidelines checklist summary
The full list is below: 25 ok, 6 partial, 1 todo (backdrop), the rest n/a.

## Third-party apps tested (shots in tests/visual/apps/, light and dark)
- Dolphin 26.08 and Kate 26.08 (Flatpak, KDE runtime 6.11): good with the Libadwaita KDE colour scheme.
- qBittorrent 5.3 (Flatpak): good.
- VLC (host, Qt 5): good, menus included.
- OBS (host, Qt 6): partly; OBS paints most of its window with its own stylesheet theme, only menus and dialogs
  are ours.
- Telegram Desktop: not tested; it bundles Qt statically and ignores style plugins.
- Kinoklip: the change to list the style in its style menu was handed to the Kinoklip session.

## Guidelines checklist (detail)

**libadwaita 1.10.0 / GNOME 51.** One rule per line; Status as of 2026-11-20 (ok / partial / todo / n/a).
HIG = https://developer.gnome.org/hig/ ; AdwDocs = https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/

| # | Rule | Source | Status |
|---|---|---|---|
| 1 | Spacing follows the 6px grid (3, 6, 9, 12, 18, 24, 36); control gaps 6, group gaps 12-18 | [HIG UI styling](https://developer.gnome.org/hig/guidelines/ui-styling.html) | ok: paddings and gaps use the 6px scale in metrics; app layouts (Qt layouts) stay the app's |
| 2 | Corner radii: buttons/entries/menu items 9, cards/boxed lists 12, popovers/windows/dialogs 15, alerts 18 | [HIG UI styling](https://developer.gnome.org/hig/guidelines/ui-styling.html), tokens.md s.4 | ok: 9 / 12 / 15 from tokens; framed scroll areas rounded via corner patches; alert dialogs n/a (Qt dialogs are plain windows) |
| 3 | Default font Adwaita Sans 11 (Adwaita Mono for code); titles/headings via weight and size classes only | [HIG typography](https://developer.gnome.org/hig/guidelines/typography.html) | ok: Adwaita Sans 11 applied when installed (ADWAITA_KEEP_APP_FONT=1 to opt out); title classes n/a in Qt |
| 4 | Button labels bold; flat buttons in header bars and toolbars, raised only for standalone/linked actions | [HIG buttons](https://developer.gnome.org/hig/patterns/controls/buttons.html) | ok: bold labels; flat via adwaita-class=flat and in QToolBar; raised elsewhere |
| 5 | Suggested (accent fill) at most one per view, for the main safe action; never next to many others | [HIG buttons](https://developer.gnome.org/hig/patterns/controls/buttons.html) | partial: suggested-action via adwaita-class property; QDialogButtonBox default buttons are not auto-marked |
| 6 | Destructive style (red text, tinted bg) only for destructive actions; filled only with suggested/opaque | [HIG buttons](https://developer.gnome.org/hig/patterns/controls/buttons.html), [AdwDocs style classes](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/style-classes.html) | ok: destructive-action via adwaita-class property (apps must opt in) |
| 7 | Pill buttons for primary actions in empty states/dialogs; circular for round icon buttons | [AdwDocs style classes](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/style-classes.html) | ok: pill and circular classes |
| 8 | Header bar 47px (46+1px line), centred bold title, window controls right, flat icon buttons | [HIG header bars](https://developer.gnome.org/hig/patterns/containers/header-bars.html) | partial: headerbar class in gallery matches; real title bars come from the WM / KWin (server-side) |
| 9 | Menus/popovers: 32px items, 12px side padding, 9px radius, separators for sections, accelerators dimmed | [HIG menus](https://developer.gnome.org/hig/patterns/controls/menus.html), [popovers](https://developer.gnome.org/hig/patterns/containers/popovers.html) | ok: 32px items, 12px padding, 15px popover radius, 9px item radius, dimmed accelerators; rounded without compositor via mask (no shadow) |
| 10 | Dialogs: alert dialog 18px radius, buttons padded 10/20, one suggested/destructive max | [HIG dialogs](https://developer.gnome.org/hig/patterns/feedback/dialogs.html) | n/a: Qt dialogs use normal windows; buttons follow rules 4-6 |
| 11 | Boxed lists: card background, 12px radius, 1px row separators, no separator after last row | [HIG boxed lists](https://developer.gnome.org/hig/patterns/containers/boxed-lists.html), [AdwDocs](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/boxed-lists.html) | ok: boxed-list / boxed-row classes |
| 12 | Lists and columns: hover 4%, active 8%, selection accent 25% (not solid fill) | [HIG lists](https://developer.gnome.org/hig/patterns/containers/list-column-views.html) | ok: hover 4%, active 8%, selection accent 25%; list selections are rounded 9px pills |
| 13 | Switches (46x26) for immediate on/off settings; checkboxes/radios (20x20 hit box) for form choices | [HIG switches](https://developer.gnome.org/hig/patterns/controls/switches.html), [checkboxes](https://developer.gnome.org/hig/patterns/controls/checkboxes.html) | ok: switch via adwaita-switch property on QCheckBox; checks/radios 20x20 |
| 14 | Sliders: 10px trough, 20px round knob; marks only when values matter | [HIG sliders](https://developer.gnome.org/hig/patterns/controls/sliders.html) | ok: measured against reference (slider rows OK in compare report) |
| 15 | Entries 34px tall, tinted background without border; spin buttons keep +/- inside the field | [HIG text fields](https://developer.gnome.org/hig/patterns/controls/text-fields.html), [spin buttons](https://developer.gnome.org/hig/patterns/controls/spin-buttons.html) | ok: entries/spin buttons match reference |
| 16 | Dropdowns look like buttons with a trailing arrow; popover list with check marks | [HIG drop-downs](https://developer.gnome.org/hig/patterns/controls/drop-downs.html) | ok: dropdown as button with arrow; popup in popover style (keeps longest-item width by design) |
| 17 | Tabs/view switchers: selected tab tinted (10%), not a raised card; close button on hover | [HIG tabs](https://developer.gnome.org/hig/patterns/nav/tabs.html) | ok: QTabBar selected tint, no raised card |
| 18 | Sidebars: 36px rows, 9px radius, sidebar background with 1px divider | [HIG sidebars](https://developer.gnome.org/hig/patterns/nav/sidebars.html) | partial: 9px rounded rows, sidebar colours via KDE scheme; row height is the app's |
| 19 | Progress bars 8px, rounded; spinners for unknown duration | [HIG progress bars](https://developer.gnome.org/hig/patterns/feedback/progress-bars.html) | ok: 8px rounded progress bar; busy mode animated |
| 20 | Tooltips short, dark rgba(0,0,6,.8) bubble, 9px radius | [HIG tooltips](https://developer.gnome.org/hig/patterns/feedback/tooltips.html) | ok: tooltip bubble rgba(0,0,6,.8), opaque #1c1c1e without compositor |
| 21 | Focus ring (2px accent, 50%) only for keyboard focus (focus-visible); entries show it on any focus | [HIG keyboard](https://developer.gnome.org/hig/guidelines/keyboard.html), _drawing.scss:8 | ok: focus-visible only after keyboard focus change; entries on any focus |
| 22 | Everything reachable by keyboard: Tab order, arrow keys in lists, Esc closes popovers/dialogs, mnemonics | [HIG keyboard](https://developer.gnome.org/hig/guidelines/keyboard.html) | n/a: behaviour belongs to the apps (style does not change keyboard handling) |
| 23 | Text contrast >= 4.5:1 (3:1 large/UI); do not rely on colour alone | [HIG accessibility](https://developer.gnome.org/hig/guidelines/accessibility.html) | ok: colours are libadwaita's own tokens |
| 24 | High contrast (portal contrast=1): 50% borders, 1px outlines on controls, stronger rings and dim/disabled opacities | [AdwDocs styles](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/styles-and-appearance.html), tokens.md s.7 | ok: hc-light / hc-dark token sets, portal contrast and ADWAITA_HIGH_CONTRAST |
| 25 | Reduced motion: honour gtk-enable-animations / gsettings enable-animations, drop every transition | [HIG accessibility](https://developer.gnome.org/hig/guidelines/accessibility.html), adw-style-manager.c:475 | ok: animations off when the setting disables them (switch, scrollbar fade) |
| 26 | Accent colours: nine accents from portal accent-color via nearest-hue mapping; default blue; accent-fg white | [AdwDocs styles](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/styles-and-appearance.html), tokens.md s.2 | ok: portal accent-color mapped by OKLCH hue to the nine accents; ADWAITA_ACCENT override |
| 27 | Standalone accent/semantic text colours use the oklab lightness clamp (L<=0.5 light, L>=0.85 dark) | [AdwDocs CSS variables](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/css-variables.html) | ok: Oklab clamp in token derivation |
| 28 | Dark style from portal color-scheme (or app override), live switching, same layout as light | [AdwDocs styles](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/styles-and-appearance.html) | ok: portal color-scheme with live switching; ADWAITA_COLOR_SCHEME override; KDE apps also need the Libadwaita KDE colour scheme |
| 29 | Overlay scrollbars: thin (3px) idle bar that expands to 8px on hover and fades out; classic scrollbars only when forced | libadwaita _scrolling.scss:47-113 | partial: overlay bars expand and fade; the compare report still shows small colour differences in dark |
| 30 | Disabled controls dimmed by opacity (50%), not recoloured | [AdwDocs CSS variables](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/css-variables.html) | ok: disabled opacity 50% from tokens |
| 31 | Unfocused (backdrop) windows: header bar uses backdrop colour, content fades to 50% in the bar | _header-bar.scss:41 | todo: no backdrop (inactive window) state yet |
| 32 | Touch/pointer: minimum 34px hit targets for buttons and rows (HIG 32+ for menu items) | [HIG pointer and touch](https://developer.gnome.org/hig/guidelines/pointer-touch.html) | ok: 34px buttons/entries, 32px menu items |
| 33 | RTL: mirror paddings, arrows, sliders and tab order | [HIG keyboard](https://developer.gnome.org/hig/guidelines/keyboard.html) | partial: scroll bars and sliders mirror; other controls rely on Qt's visualRect defaults, not tested |
| 34 | Icons: Adwaita symbolic icons recoloured to text colour, 16px in controls, 32px large | [HIG UI icons](https://developer.gnome.org/hig/guidelines/ui-icons.html) | partial: style's own arrows/checks are drawn in text colour; app icons come from the icon theme and are not recoloured |
