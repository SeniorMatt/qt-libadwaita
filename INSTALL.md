# Install

Requires Qt 6 (Widgets, DBus; public API only, no private headers), CMake >= 3.16, a C++17 compiler.

## Build and install to ~/.local
```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build -j4
cmake --install build
```
This installs `qtlibadwaitastyle.so` into `<prefix>/<libdir>/qt6/plugins/styles` (e.g. `~/.local/lib/qt6/plugins/styles`)
and `adwaita-gallery` into `~/.local/bin`. Use `-DADWAITA_PLUGIN_DIR=lib/x86_64-linux-gnu/qt6/plugins/styles` for
another layout relative to the prefix, or an absolute `-DQT_PLUGIN_INSTALL_DIR=/app/lib/plugins/styles`
(KDE_INSTALL_QTPLUGINDIR style, e.g. for a Flatpak extension; overrides everything else).

Qt does not search `~/.local` by default, so point it there:
```sh
export QT_PLUGIN_PATH=$HOME/.local/lib/qt6/plugins   # add to ~/.profile or ~/.config/environment.d/qt.conf
```

## Enable
The style key is **Libadwaita** (the key "Adwaita" belongs to the old adwaita-qt).
- Per app / session: `QT_STYLE_OVERRIDE=Libadwaita myapp`, or `myapp -style Libadwaita`.
- Everywhere: put `QT_STYLE_OVERRIDE=Libadwaita` in `~/.config/environment.d/qt.conf`.
- qt6ct: set Style to `Libadwaita` (run apps with `QT_QPA_PLATFORMTHEME=qt6ct`).
- KDE: System Settings > Appearance > Application Style > Libadwaita. See [docs/install-kde-linux.md](docs/install-kde-linux.md).

Dark/light, accent colour and high contrast follow the GNOME settings (xdg-desktop-portal) live. Testing overrides:
`ADWAITA_COLOR_SCHEME=light|dark`, `ADWAITA_ACCENT=blue|teal|green|yellow|orange|red|pink|purple|slate|#rrggbb`,
`ADWAITA_HIGH_CONTRAST=0|1`, `ADWAITA_ANIMATIONS=0|1`.

## Window decorations and font
- Wayland title bars: install [QAdwaitaDecorations](https://github.com/FedoraQt/QAdwaitaDecorations) and run with
  `QT_WAYLAND_DECORATION=adwaita`.
- Font: the current GNOME default is Adwaita Sans (package `adwaita-fonts`); select it in your desktop font settings.

## App-facing API (dynamic properties, mirroring GTK style classes)
| Property | Value | Effect |
|---|---|---|
| `adwaita-class` | `suggested-action` | accent-filled button (a QPushButton that is `default` looks the same) |
| `adwaita-class` | `destructive-action` | red button |
| `adwaita-class` | `flat` | transparent button (also `QPushButton::setFlat`) |
| `adwaita-class` | `pill` | fully rounded button |
| `adwaita-class` | `headerbar` | plain QWidget painted as a header bar |
| `adwaita-class` | `boxed-list` / `boxed-row` | card frame / row (+ `adwaita-row-index` int: separator above rows > 0) |
| `adwaita-switch` | `true` on a QCheckBox | draws a switch |
| `adwaita-focus-visible` | `true` | force the focus ring (testing) |

Example: `button->setProperty("adwaita-class", "suggested-action");`

## Widget gallery and screenshots
```sh
adwaita-gallery                                  # interactive
adwaita-gallery --shot out --scale 2             # PNGs <section>@2x.png (scheme via ADWAITA_* env)
tools/shoot_qt.sh ~/.local/bin/adwaita-gallery   # full matrix into docs/qt-shots/<scheme>/<accent>/
```
Token data: `python3 tools/gen_tokens.py` regenerates `src/adwaitatokens_data.h` from `data/adwaita-1.10.json`
(CMake does this automatically when the JSON changes).

## Qt 5.15 build

Same sources, compile-time variant: `cmake -S . -B build-qt5 -DQT_MAJOR=5 -DCMAKE_INSTALL_PREFIX=$HOME/.local && cmake --build build-qt5`.
Installs `qtlibadwaitastyle.so` into `<prefix>/lib/qt5/plugins/styles` (key "Libadwaita"); the gallery is named `adwaita-gallery-qt5`.
Use it with Qt 5 apps via `QT_STYLE_OVERRIDE=Libadwaita` (and `QT_PLUGIN_PATH` if not installed in a Qt plugin dir).

## KDE apps: also pick the matching colour scheme
KDE apps (Dolphin, Kate, Okular...) paint their content areas from the KDE colour scheme, not only from the
style. Install the schemes and select "Libadwaita" or "Libadwaita Dark" in System Settings > Colours:

    mkdir -p ~/.local/share/color-schemes && cp kde-colors/*.colors ~/.local/share/color-schemes/

Regenerate them from the tokens with `python3 tools/gen_kde_colors.py`. The style keeps its own palette and uses
Adwaita Sans 11 when installed; set ADWAITA_KEEP_APP_PALETTE=1 or ADWAITA_KEEP_APP_FONT=1 to keep the app's.

## Spacing and font size
Create `~/.config/qt-libadwaita.conf` (restart the apps after a change):

    [General]
    spacing=comfortable   # compact | normal | comfortable | any number 0.5-3 (gaps, margins, list rows)
    font-size=11          # Adwaita Sans size; default 10 (fits Qt/KDE layouts), 11 = exact GNOME size, 0 = app font

Or use the env vars `ADWAITA_SPACING` and `ADWAITA_FONT_SIZE`. Flatpak apps can't read that file; use
`flatpak override --user --env=ADWAITA_SPACING=comfortable <app-id>` instead.
