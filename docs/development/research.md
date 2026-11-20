# Step 1 notes (2026-11-20) - move into the repo's docs/ once it exists

## Versions followed
- libadwaita **1.10.0** (tag 2026-09-14; newest stable; 1.9.4 / 1.8.8 are point releases of older branches)
- GNOME **51** (latest point release 51.1, 2026-10-14). Flatpak runtime org.gnome.Platform//51 is installed here.
- Default fonts (gsettings-desktop-schemas main): "Adwaita Sans 11", "Adwaita Mono 11"; icon theme "Adwaita";
  default accent "blue". Adwaita Sans is NOT installed on this PC system-wide (fc-list) - ship/recommend it.
- 1.10 cycle style changes: GtkDropDown gets .flat/.raised; lighter icon weight for large icons; improved popover
  shadow; AdwStyleManager speedups / portal timeout. No palette or radius overhaul vs 1.9.
- Local Qt: 6.4.2 (qt6-base-dev). qt6-base-private-dev is not installed (needed only for a platform theme plugin).

## Existing projects - what we take
| Project | Licence / state | Take | Leave |
|---|---|---|---|
| adwaita-qt (FedoraQt) | style GPL-2.0+, lib LGPL-2.1+; archived 2023; Qt5 + Qt6 option; ~8k lines, Breeze-derived | Code structure: QStyle subclass + helper/renderer split, animation engines, window-drag manager, splitter proxy, mnemonics, widget-factory demo | All colours/metrics (GTK3-era Adwaita SCSS), its "Adwaita" visuals |
| QGnomePlatform (FedoraQt) | LGPL-2.1; archived 2023 | How to read settings: xdg-desktop-portal Settings (Read/SettingChanged) with gsettings fallback; font/icon/dialog hints | GTK3 colour schemes, its decoration code (superseded by QAdwaitaDecorations) |
| KvLibadwaita | GPL-3.0; active (2025) | Nothing in code; at most a cross-check of geometry. Kvantum SVG approach is not used | Kvantum dependency; visuals derived from Colloid |
| QAdwaitaDecorations (FedoraQt) | LGPL-2.1; active (2026-08) | Use as-is for Wayland client-side decorations; document QT_WAYLAND_DECORATION=adwaita | No fork needed |

## Decision: start fresh (not revive adwaita-qt)
- adwaita-qt's palette, radii, sizes and flat/raised logic are GTK3-era; nearly every drawing function would be
  rewritten for libadwaita 1.10 anyway, and its archived state means no upstream to merge with.
- Fresh code keeps one clear source of truth (libadwaita 1.10 SCSS -> generated colour/metric tables).
- We still reuse small, generic helpers from adwaita-qt/Breeze (animations, window drag, splitter proxy), with
  copyright headers kept and credited in AUTHORS/README. Hence licence GPL-2.0-or-later (compatible with all).
- Platform side: rely on xdg-desktop-portal directly inside the style (accent, color-scheme, contrast) so it works
  without any platform theme; platform theme plugin optional later (needs qt6-base-private-dev).
