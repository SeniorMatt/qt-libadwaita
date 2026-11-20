#!/bin/sh
# Reshoot the Qt gallery (offscreen, ~2 s for the whole matrix) and regenerate tests/visual/report/{sheets,REPORT.md}.
# Usage: tests/visual/compare/run.sh [gallery-binary]      env: STYLE_SO=path/to/qtlibadwaitastyle.so  SHOOT=0 (skip reshoot)
# Needs numpy + Pillow: uses python3 if it has both, else a venv in ~/.cache/qt-adwaita-compare (created once with uv or venv+pip).
set -e
root=$(cd "$(dirname "$0")/../../.." && pwd)
work=$(dirname "$root")                       # .../qt-adwaita-work
gallery=${1:-${GALLERY:-$work/build/adwaita-gallery}}
so=${STYLE_SO:-$(dirname "$gallery")/qtlibadwaitastyle.so}
py=python3
if ! $py -c "import numpy, PIL" 2>/dev/null; then
  venv=${COMPARE_VENV:-$HOME/.cache/qt-adwaita-compare}
  if [ ! -x "$venv/bin/python" ] || ! "$venv/bin/python" -c "import numpy, PIL" 2>/dev/null; then
    if command -v uv >/dev/null; then uv venv -q --clear "$venv" && uv pip install -q -p "$venv/bin/python" numpy pillow
    else python3 -m venv "$venv" && "$venv/bin/pip" -q install numpy pillow; fi
  fi
  py=$venv/bin/python
fi
if [ "${SHOOT:-1}" != 0 ]; then
  [ -x "$gallery" ] || { echo "gallery not found: $gallery (pass it as argument)" >&2; exit 1; }
  [ -f "$so" ] || { echo "style plugin not found: $so (set STYLE_SO)" >&2; exit 1; }
  plug=$(mktemp -d); mkdir "$plug/styles"; ln -s "$so" "$plug/styles/"
  QT_QPA_PLATFORM=offscreen QT_PLUGIN_PATH=$plug "$root/tools/shoot_qt.sh" "$gallery" 2>&1 | grep -v propagateSizeHints || true
  rm -rf "$plug"
fi
"$py" "$root/tests/visual/compare/compare.py" 2>&1 | tail -n 30
echo "report: $root/tests/visual/report/REPORT.md"
