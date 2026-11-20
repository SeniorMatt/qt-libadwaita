# Updating to a new libadwaita release

The style takes every colour and metric from one token file, `data/adwaita-<version>.json`; `data/current` names the active one.
A script rebuilds that file from the libadwaita stylesheet sources, so a GNOME release is mostly one command plus a visual check.

## 1. Look
```sh
tools/update_libadwaita.py --check                 # current=1.10.0 latest=1.10.0 newer=false
tools/update_libadwaita.py latest --dry-run        # (or a tag like 1.11.0, or main) report only, nothing written in the repo
```
The report lists changed values, new/removed keys, warnings and the manual keys (below). Downloads (a few tens of kB) and the
compiler venv are cached in `$LIBADWAITA_CACHE` (default `<tmp>/qt-libadwaita-update`, or `--cache DIR`). A Sass compiler is
needed: `sassc`, `sass`, or Python `libsass` (installed by the script into `--user` or a venv in the cache if missing).

## 2. Apply
```sh
tools/update_libadwaita.py 1.11.0 --apply          # --no-build to skip the build, --gnome 52 to record the GNOME release
```
It writes `data/adwaita-1.11.json`, points `data/current` at it, regenerates `src/adwaitatokens_data.h` and
`data/kde-colors/*.colors`, builds with `-j4` in `<cache>/build` and lists the changed files. Nothing is committed.
A patch release of the same minor version (1.10.1) rewrites `adwaita-1.10.json` in place. If a selector it expects has
disappeared upstream, it keeps the old value and prints a warning: fix the extraction in `tools/update_libadwaita.py` (`build()`).

The same thing runs weekly in CI (`.github/workflows/libadwaita-update.yml`): when a newer release tag exists it opens a pull
request with the regenerated files and the value diff. The repository setting **Settings > Actions > General > Workflow
permissions > "Allow GitHub Actions to create and approve pull requests"** must be enabled, or the last step fails.

## 3. Check by eye
1. Read the report: every changed value, every new colour (new keys are added to the JSON but the style code only uses names
   listed in `src/adwaitatokens.h`), every warning.
2. Gallery: `build/adwaita-gallery --style Libadwaita`, light and dark, high contrast (`ADWAITA_*` env, see
   [development.md](development.md)), a couple of accents.
3. Visual comparison against real GTK: make fresh reference shots of the new version, then compare
   ```sh
   FORCE=1 tests/visual/reference-tools/shoot_all.sh   # needs flatpak org.gnome.Platform//<new GNOME runtime>
   tests/visual/compare/run.sh                         # writes tests/visual/report/REPORT.md and sheets
   ```
   Update `tests/visual/reference/README.md` (versions) after reshooting. Look at the sheets of the widgets whose values changed.
4. New widgets or restyled ones (new rules in the upstream SCSS) are not picked up by the script: skim the upstream
   `src/stylesheet/widgets/*.scss` diff between the two tags.
5. Update the prose that names the version: `docs/development/tokens.md` heading and line references, `docs/development/research.md`,
   `docs/status.md`, the README badge.

## Which values are manual
The script derives about 370 values from the stylesheet; the rest of the JSON (about 600) is carried over unchanged and
printed as "Manual" in the report:
- derived: accent palette and standalone (text) colours, all scheme colours, opacities, base palette, overlay alphas, OSD/toast/tooltip
  colours, the radii, and the metrics of scrollbar, scale, progress bar, spin button and the basic button box.
- manual: everything else under `widgets/` (hand-measured geometry of entries, switches, headerbar, lists, popovers, tabs ...),
  `typography`, `transitions`, `focus_ring`, `high_contrast`, `osd_overrides`, `animations`, `scheme_notes`, prose strings,
  all `src` file:line references (they drift with every release), the accent portal mapping and `accent_fg`/default accent
  (libadwaita C code, `adw-accent-color.c`, not the stylesheet), `radii/toggle_group_inner` (a `calc()`), and `_meta/gnome`.
Compare these against the upstream SCSS when the report or the visual comparison points at a widget.
