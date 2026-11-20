#!/bin/bash
# Renders the whole libadwaita reference matrix into docs/reference/.
# Needs: flatpak with org.gnome.Platform//51, host Xvfb. Nothing is installed or changed on the host;
# a private Xvfb is used so no window appears on the desktop and no host settings are touched.
# Usage: shoot_all.sh [scheme ...]   (default: light dark hc-light hc-dark)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
OUTROOT="$(cd "$HERE/../../docs/reference" && pwd)"
SCHEMES=("${@:-light dark hc-light hc-dark}"); [ $# -eq 0 ] && SCHEMES=(light dark hc-light hc-dark)
DISP=:97
for scheme in "${SCHEMES[@]}"; do
  case $scheme in
    light) CS=prefer-light; HC=0;; dark) CS=prefer-dark; HC=0;;
    hc-light) CS=prefer-light; HC=1;; hc-dark) CS=prefer-dark; HC=1;;
  esac
  for accent in blue orange; do
    for scale in 1 2; do
      out="$OUTROOT/$scheme/$accent"; mkdir -p "$out"
      [ -f "$out/menu-shadow@${scale}x.png" ] && [ -z "${FORCE:-}" ] && { echo "skip $scheme $accent @${scale}x"; continue; }
      for try in 1 2 3; do
        Xvfb $DISP -ac -screen 0 1400x2600x24 >/dev/null 2>&1 & XP=$!
        for i in $(seq 1 20); do DISPLAY=$DISP xdpyinfo >/dev/null 2>&1 && break; read -t 0.5 <> <(:) 2>/dev/null; done
        DISPLAY=$DISP timeout 120 flatpak run --nosocket=wayland --socket=x11 --share=ipc \
          --filesystem="$HERE:ro" --filesystem="$out" \
          --env=GDK_BACKEND=x11 --env=GSK_RENDERER=cairo --env=GDK_DISABLE=gl,vulkan --env=GTK_A11Y=none \
          --env=GDK_SCALE=$scale --env=ADW_DEBUG_COLOR_SCHEME=$CS --env=ADW_DEBUG_HIGH_CONTRAST=$HC \
          --env=ADW_DEBUG_ACCENT_COLOR=$accent \
          --command=python3 org.gnome.Platform//51 "$HERE/adw_gallery.py" "$out" 2>&1 | grep -E "^(dark|libadw)|rror|Traceback"
        kill $XP; wait $XP 2>/dev/null
        [ -f "$out/menu-shadow@${scale}x.png" ] && break
      done
      echo "done $scheme $accent @${scale}x"
    done
  done
  for accent in blue orange; do
    flatpak run --filesystem="$HERE:ro" --filesystem="$OUTROOT" --command=python3 org.gnome.Platform//51 \
      "$HERE/contact_sheet.py" "$OUTROOT/$scheme/$accent" "$OUTROOT/sheet-$scheme-$accent.png" "$scheme $accent"
  done
done
