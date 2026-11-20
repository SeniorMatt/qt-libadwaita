# Development

## Build
```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build -j4
```
Qt 5.15 variant: add `-DQT_MAJOR=5` (separate build directory). Needs CMake >= 3.16, a C++17 compiler, Qt Widgets and
DBus; public Qt API only. Installing: [install.md](install.md).

To try a build without installing, point Qt at it:
`QT_PLUGIN_PATH=<dir containing styles/qtlibadwaitastyle.so> build/adwaita-gallery --style Libadwaita`.

## Code layout
- `src/adwaitastyle.cpp/.h`: the QStyle class, shared helpers and dispatch.
- `src/adwaitastyle_<family>.cpp`: one file per widget family: `buttons`, `checks`, `entries`, `focus`, `menus`,
  `range` (sliders, progress), `scroll`, `tabs`, `views` (lists, trees, tables, headers).
- `src/adwaitaanimations.*`: the shared transition engine (hover/press/focus fades, 16 ms timer only while something moves;
  `ADWAITA_ANIMATIONS=0`, GNOME enable-animations or KDE AnimationDurationFactor=0 turn it off). Paint code asks
  `AdwAnim::color/value(widget, key, target)` for the displayed value; no Q_OBJECT classes, so no moc needed.
- `src/adwaitasettings.*`: reads portal / KDE / env settings (scheme, accent, contrast, animations, spacing, font).
- `src/adwaitatokens.*`: colour and metric tokens; `adwaitatokens_data.h` is generated (see below).
- `src/adwaitastyleplugin.*`, `src/adwaita.json`: the plugin entry point and its key ("Libadwaita").
- `gallery/main.cpp`: the widget gallery app (`adwaita-gallery`).

## Design tokens
`data/current` names the active token file (`data/adwaita-1.10.json`, the machine source for colours and metrics measured from
libadwaita 1.10). CMake, `tools/gen_tokens.py` and `tools/gen_kde_colors.py` all read it, so switching version is one step.
`python3 tools/gen_tokens.py` regenerates `src/adwaitatokens_data.h` (CMake does this when the JSON changes).
`python3 tools/gen_kde_colors.py` writes the KDE colour schemes in `data/kde-colors/`. Human-readable token
reference: [development/tokens.md](development/tokens.md). Background and decisions:
[development/research.md](development/research.md).

When GNOME releases a new libadwaita: `tools/update_libadwaita.py <tag> --apply`, see
[updating-libadwaita.md](updating-libadwaita.md).

## Gallery and screenshots
```sh
adwaita-gallery                                  # interactive
adwaita-gallery --shot out --scale 2             # PNGs <section>@2x.png (scheme via ADWAITA_* env)
tools/shoot_qt.sh build/adwaita-gallery          # full matrix into tests/visual/qt-shots/<scheme>/<accent>/
python3 tools/make_readme_images.py              # compose docs/readme/*.png from those shots
```

## Visual comparison against real GTK
Everything lives in `tests/visual/`:
- `reference/`: shots of real libadwaita 1.10 (4 schemes x 2 accents); how they were made: `reference/README.md`.
- `reference-tools/`: the scripts that make them (Flatpak GNOME runtime, private Xvfb): `shoot_all.sh`.
- `compare/run.sh [gallery-binary]`: reshoots the Qt gallery offscreen, then `compare.py` cuts both shots into widget
  rectangles, matches them and writes sheets and `report/REPORT.md`. Needs numpy + Pillow (a venv is made if missing).
  Without an argument it uses `../build/adwaita-gallery` (next to the repo); `STYLE_SO=path` picks the plugin,
  `SHOOT=0` skips the reshoot.
- `report/`: generated report, metrics and side-by-side sheets `[libadwaita | Qt | difference]`.
- `apps/`: screenshots of third-party apps (light and dark) and the scripts that made them.
- `qt-shots/`: output of the gallery (gitignored).

## Status
What is done and the known differences: [status.md](status.md).
