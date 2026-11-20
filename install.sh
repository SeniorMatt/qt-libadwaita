#!/bin/sh
# qt-libadwaita preview installer (user only, no root).
#   ./install.sh            install style plugins (Qt 6 + Qt 5), KDE colour schemes, Adwaita fonts,
#                           and the Flatpak extension for Flatpak apps (if flatpak/ bundles are present)
#   ./install.sh --enable   also make every Qt app use it (host: QT_STYLE_OVERRIDE in the session; Flatpak: override)
#   ./install.sh --remove   undo everything
set -e
here=$(cd "$(dirname "$0")" && pwd)
L=$HOME/.local
envfile=$HOME/.config/plasma-workspace/env/qt-libadwaita.sh
envd=$HOME/.config/environment.d/qt-libadwaita.conf
if [ "$1" = "--remove" ]; then
  rm -f "$L/lib/qt6/plugins/styles/qtlibadwaitastyle.so" "$L/lib/qt5/plugins/styles/qtlibadwaitastyle.so" \
        "$L/share/color-schemes/Libadwaita"*.colors "$envfile" "$envd"
  rm -rf "$L/share/fonts/Adwaita-qt-libadwaita"
  if command -v flatpak >/dev/null; then
    g=$L/share/flatpak/overrides/global  # drop only the lines this script added
    [ -f "$g" ] && sed -i -e "s#$L/lib/qt6/plugins:ro;##" -e '/^filesystems=$/d' \
        -e "\#^QT_PLUGIN_PATH=$L/lib/qt6/plugins#d" -e '/^QT_STYLE_OVERRIDE=Libadwaita$/d' "$g"
    for b in $(flatpak list --user --runtime --columns=ref | grep '^org.kde.KStyle.Libadwaita/'); do flatpak uninstall --user -y "$b"; done
  fi
  echo "removed; log out and back in"; exit 0
fi
mkdir -p "$L/lib/qt6/plugins/styles" "$L/lib/qt5/plugins/styles" "$L/share/color-schemes"
# Replace files atomically (copy to a temp name, then rename). Running programs (KWin, plasmashell, every Qt app)
# have the old plugin and fonts memory-mapped; writing over them in place crashes those programs instantly.
put() {  # put SRC DIR
  dst="$2/$(basename "$1")"
  cmp -s "$1" "$dst" && return 0
  cp "$1" "$dst.new.$$" && mv -f "$dst.new.$$" "$dst"
}
put "$here/qt6/qtlibadwaitastyle.so" "$L/lib/qt6/plugins/styles"
put "$here/qt5/qtlibadwaitastyle.so" "$L/lib/qt5/plugins/styles"
for f in "$here/color-schemes/"*.colors; do put "$f" "$L/share/color-schemes"; done
# Adwaita Sans / Mono (SIL OFL, see fonts/LICENSE); skipped if the system already has them.
if ! fc-list 2>/dev/null | grep -q "Adwaita Sans"; then
  mkdir -p "$L/share/fonts/Adwaita-qt-libadwaita"
  for f in "$here/fonts/"*; do put "$f" "$L/share/fonts/Adwaita-qt-libadwaita"; done
  fc-cache -f "$L/share/fonts" >/dev/null 2>&1 || true
fi
mkdir -p "$(dirname "$envfile")" "$(dirname "$envd")"
# Qt must find the plugin in ~/.local, also for Plasma's "Application Style" list.
cat > "$envfile" <<EOT
export QT_PLUGIN_PATH=\$HOME/.local/lib/qt6/plugins:\$HOME/.local/lib/qt5/plugins\${QT_PLUGIN_PATH:+:\$QT_PLUGIN_PATH}
EOT
printf 'QT_PLUGIN_PATH=%s/.local/lib/qt6/plugins:%s/.local/lib/qt5/plugins\n' "$HOME" "$HOME" > "$envd"
if [ "$1" = "--enable" ]; then
  echo 'export QT_STYLE_OVERRIDE=Libadwaita' >> "$envfile"
  echo 'QT_STYLE_OVERRIDE=Libadwaita' >> "$envd"
fi
# Flatpak apps. Default route: let every Flatpak app read the plugin from ~/.local (read-only) through a user
# override; works with any KDE runtime 6.x. If flatpak/*.flatpak extension bundles are shipped, install those too.
if command -v flatpak >/dev/null; then
  flatpak override --user --filesystem="$L/lib/qt6/plugins:ro" --env=QT_PLUGIN_PATH="$L/lib/qt6/plugins:/app/lib/plugins:/app/plugins"
  echo "Flatpak apps can now load the style (override in $L/share/flatpak/overrides/global)"
  if ls "$here"/flatpak/*.flatpak >/dev/null 2>&1; then
    for b in "$here"/flatpak/*.flatpak; do
      br=$(basename "$b" .flatpak); br=${br##*-}
      if flatpak info --user org.kde.Platform//"$br" >/dev/null 2>&1 || flatpak info --system org.kde.Platform//"$br" >/dev/null 2>&1; then
        flatpak install --user -y --reinstall --bundle "$b" >/dev/null && echo "flatpak extension for KDE runtime $br installed"
      fi
    done
  fi
  [ "$1" = "--enable" ] && flatpak override --user --env=QT_STYLE_OVERRIDE=Libadwaita
fi
echo
echo "Installed. Log out and back in, then:"
echo "  System Settings > Colours & Themes > Application Style: Libadwaita"
echo "  System Settings > Colours & Themes > Colours: Libadwaita or Libadwaita Dark"
echo "Try one app first without logging out:"
echo "  QT_PLUGIN_PATH=$L/lib/qt6/plugins QT_STYLE_OVERRIDE=Libadwaita kate"
echo "  flatpak run --env=QT_STYLE_OVERRIDE=Libadwaita org.kde.kate"
