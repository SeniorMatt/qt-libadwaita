#!/bin/sh
# Render the gallery matrix: light/dark/hc-light/hc-dark x blue/orange x 1x/2x -> tests/visual/qt-shots/<scheme>/<accent>/
# Usage: tools/shoot_qt.sh [path/to/adwaita-gallery]   (the style plugin must be found: QT_PLUGIN_PATH)
set -e
root=$(cd "$(dirname "$0")/.." && pwd)
gallery=${1:-adwaita-gallery}
# Reference font (Adwaita Sans/Mono) from the GNOME 51 runtime, read-only; override with ADWAITA_GALLERY_FONT_DIR.
if [ -z "${ADWAITA_GALLERY_FONT_DIR+x}" ]; then
  loc=$(flatpak info --show-location org.gnome.Platform//51 2>/dev/null || true)
  [ -n "$loc" ] && [ -d "$loc/files/share/fonts/Adwaita" ] && ADWAITA_GALLERY_FONT_DIR=$loc/files/share/fonts/Adwaita
fi
export ADWAITA_GALLERY_FONT_DIR
export ADWAITA_FONT_SIZE=11  # references are GNOME at 11 pt (the style default is 10)
for scheme in light dark hc-light hc-dark; do
  case $scheme in light) cs=light hc=0;; dark) cs=dark hc=0;; hc-light) cs=light hc=1;; hc-dark) cs=dark hc=1;; esac
  for accent in blue orange; do
    for scale in 1 2; do
      ADWAITA_COLOR_SCHEME=$cs ADWAITA_HIGH_CONTRAST=$hc ADWAITA_ACCENT=$accent \
        "$gallery" --style Libadwaita --scale $scale --shot "$root/tests/visual/qt-shots/$scheme/$accent"
    done
  done
done
