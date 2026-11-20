#!/bin/sh
# qt-libadwaita installer (no root needed by default).
#
# Works in two situations:
#   * from a git checkout: builds the style plugin for Qt 6 (and Qt 5 if its dev files exist) and installs it
#   * from an unpacked release tarball (it contains qt6/ and qt5/ with prebuilt plugins): no compiler needed
#
# Run ./install.sh --help for the options.
set -e

usage() {
  cat <<'EOT'
Usage: ./install.sh [options]

  (no option)    install the style plugin (Qt 6, and Qt 5 if available), the KDE colour schemes, the
                 Adwaita fonts and the Plasma theme parts (Global Themes "Libadwaita" and "Libadwaita Dark",
                 window decorations, Plasma Style, splash, icon themes) into ~/.local; nothing is switched on yet
  --enable       also make every Qt app use the style (QT_STYLE_OVERRIDE=Libadwaita in your session,
                 and as a Flatpak override)
  --plasma       also apply the light Global Theme now (plasma-apply-lookandfeel) and register Libadwaita and
                 Libadwaita Dark as the light/dark pair for Plasma's automatic switching; run it on Plasma
  --plasma-auto  like --plasma, and also turn automatic light/dark switching on
  --no-plasma    do not install the Plasma theme parts (Global Themes, window decorations, Plasma Style, icons)
  --remove       undo everything this script installed (alias: --uninstall)
  --system       install the plugin and colour schemes into the distro's Qt plugin directory instead of
                 ~/.local (uses sudo; no QT_PLUGIN_PATH needed afterwards)
  --no-fonts     do not install the Adwaita Sans / Mono fonts
  --no-qt5       skip the Qt 5 plugin
  -h, --help     show this text

From a git checkout the script first checks for cmake, a C++17 compiler and the Qt 6 development
files, and prints the install command for your distribution if something is missing.
Details: docs/install.md
EOT
}

enable=0 remove=0 system=0 fonts=1 qt5=1 plasma=1 plasma_apply=0 plasma_auto=0
for a in "$@"; do
  case $a in
    --enable) enable=1 ;;
    --remove|--uninstall) remove=1 ;;
    --system) system=1 ;;
    --no-fonts) fonts=0 ;;
    --no-qt5) qt5=0 ;;
    --no-plasma) plasma=0 ;;
    --plasma) plasma_apply=1 ;;
    --plasma-auto) plasma_apply=1; plasma_auto=1 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $a" >&2; usage >&2; exit 2 ;;
  esac
done

here=$(cd "$(dirname "$0")" && pwd)
L=$HOME/.local
cfg=${XDG_CONFIG_HOME:-$HOME/.config}
envfile=$cfg/plasma-workspace/env/qt-libadwaita.sh
envd=$cfg/environment.d/qt-libadwaita.conf
fontdir=$L/share/fonts/Adwaita-qt-libadwaita
LNF_LIGHT=io.github.seniormatt.libadwaita
LNF_DARK=io.github.seniormatt.libadwaita-dark
# everything the Plasma part installs (all inside ~/.local/share, never needs root)
plasma_remove() {
  rm -rf "$L/share/plasma/look-and-feel/$LNF_LIGHT" "$L/share/plasma/look-and-feel/$LNF_DARK" \
         "$L/share/plasma/desktoptheme/Libadwaita" "$L/share/aurorae/themes/Libadwaita" "$L/share/aurorae/themes/Libadwaita-Dark" \
         "$L/share/icons/Libadwaita" "$L/share/icons/Libadwaita-Dark"
}
FONTS_VERSION=51.0
FONTS_URL=https://download.gnome.org/sources/adwaita-fonts/51/adwaita-fonts-$FONTS_VERSION.tar.xz
FONTS_SHA256=fa104ae2c1b96580d322f563ffe8b2dfca5296b0eca4971efd2d8011405123d2

say() { printf '%s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }

# ---- system mode helpers: find the distro's Qt plugin directories ----
qt_plugin_dir() {  # qt_plugin_dir 6|5
  if [ "$1" = 6 ]; then
    for t in qtpaths6 qtpaths-qt6; do have $t && { $t --plugin-dir 2>/dev/null && return 0; }; done
    for t in qmake6 qmake-qt6; do have $t && { $t -query QT_INSTALL_PLUGINS 2>/dev/null && return 0; }; done
  else
    for t in qtpaths-qt5 qmake-qt5 qmake5; do
      have $t || continue
      case $t in qtpaths*) $t --plugin-dir 2>/dev/null && return 0 ;; *) $t -query QT_INSTALL_PLUGINS 2>/dev/null && return 0 ;; esac
    done
    if have qmake && qmake -query QT_VERSION 2>/dev/null | grep -q '^5\.'; then qmake -query QT_INSTALL_PLUGINS; return 0; fi
  fi
  return 1
}

if [ $system = 1 ]; then
  if [ "$(id -u)" = 0 ]; then SUDO=; else have sudo || die "--system needs sudo"; SUDO=sudo; fi
  P6=$(qt_plugin_dir 6 || true); P5=$(qt_plugin_dir 5 || true)
  [ -n "$P6" ] || [ $remove = 1 ] || die "cannot find the Qt 6 plugin directory (is qtpaths6 or qmake6 installed?)"
  P6=${P6:+$P6/styles}; P5=${P5:+$P5/styles}
  SCHEMES=/usr/share/color-schemes
else
  SUDO=
  P6=$L/lib/qt6/plugins/styles; P5=$L/lib/qt5/plugins/styles
  SCHEMES=$L/share/color-schemes
fi

# ---- remove ----
if [ $remove = 1 ]; then
  if [ $system = 1 ]; then
    for d in "$P6" "$P5"; do [ -n "$d" ] && $SUDO rm -f "$d/qtlibadwaitastyle.so"; done
    $SUDO rm -f "$SCHEMES/Libadwaita"*.colors
  fi
  # always clean the user-level install too
  rm -f "$L/lib/qt6/plugins/styles/qtlibadwaitastyle.so" "$L/lib/qt5/plugins/styles/qtlibadwaitastyle.so" \
        "$L/share/color-schemes/Libadwaita"*.colors "$envfile" "$envd"
  rm -rf "$fontdir"
  plasma_remove
  if have kreadconfig6 && have kwriteconfig6; then  # drop the light/dark pair only if it is ours
    for k in DefaultLightLookAndFeel:$LNF_LIGHT DefaultDarkLookAndFeel:$LNF_DARK; do
      [ "$(kreadconfig6 --file kdeglobals --group KDE --key "${k%%:*}" 2>/dev/null)" = "${k#*:}" ] \
        && kwriteconfig6 --file kdeglobals --group KDE --key "${k%%:*}" --delete
    done || true
  fi
  if have flatpak; then
    g=$L/share/flatpak/overrides/global  # drop only the lines this script added
    [ -f "$g" ] && sed -i -e "s#$L/lib/qt6/plugins:ro;##" -e '/^filesystems=$/d' \
        -e "\#^QT_PLUGIN_PATH=$L/lib/qt6/plugins#d" -e '/^QT_STYLE_OVERRIDE=Libadwaita$/d' "$g"
    for b in $(flatpak list --user --runtime --columns=ref | grep '^org.kde.KStyle.Libadwaita/'); do flatpak uninstall --user -y "$b"; done
  fi
  say "removed; log out and back in"; exit 0
fi

# ---- which situation are we in? ----
if [ -f "$here/qt6/qtlibadwaitastyle.so" ]; then
  mode=prebuilt
  src6=$here/qt6/qtlibadwaitastyle.so
  src5=$here/qt5/qtlibadwaitastyle.so
  schemes_src=$here/color-schemes
elif [ -f "$here/CMakeLists.txt" ] && [ -d "$here/src" ]; then
  mode=source
  schemes_src=$here/data/kde-colors
else
  die "run this script from a git checkout or from an unpacked release tarball"
fi

# ---- source mode: check the toolchain, then build ----
distro_hint() {  # prints the install command for this distro
  id= like=
  if [ -r /etc/os-release ]; then id=$(. /etc/os-release; echo "$ID"); like=$(. /etc/os-release; echo "$ID_LIKE"); fi
  s=; [ "$(id -u)" = 0 ] || s="sudo "
  case " $id $like " in
    *" debian "*|*" ubuntu "*|*" linuxmint "*|*" pop "*)
      echo "${s}apt install build-essential cmake qt6-base-dev   # Qt 5 too (optional): qtbase5-dev" ;;
    *" fedora "*|*" rhel "*|*" centos "*)
      echo "${s}dnf install gcc-c++ cmake qt6-qtbase-devel   # Qt 5 too (optional): qt5-qtbase-devel" ;;
    *" arch "*|*" manjaro "*|*" endeavouros "*)
      echo "${s}pacman -S --needed base-devel cmake qt6-base   # Qt 5 too (optional): qt5-base" ;;
    *" suse "*|*" opensuse "*|*" opensuse-tumbleweed "*|*" opensuse-leap "*|*" sles "*)
      echo "${s}zypper install gcc-c++ cmake qt6-base-devel   # Qt 5 too (optional): libqt5-qtbase-devel" ;;
    *" void "*)
      echo "${s}xbps-install -S base-devel cmake qt6-base-devel   # Qt 5 too (optional): qt5-devel" ;;
    *" alpine "*)
      echo "${s}apk add build-base cmake qt6-qtbase-dev   # Qt 5 too (optional): qt5-qtbase-dev" ;;
    *) return 1 ;;
  esac
}

missing_help() {
  echo >&2
  if [ -e /run/ostree-booted ] || { [ -r /etc/os-release ] && grep -qiE '^(ID|NAME)=.*(kde.?linux|silverblue|kinoite|bazzite|nixos|steamos)' /etc/os-release; }; then
    echo "This looks like an immutable distribution. Build in a container, or use a release tarball." >&2
    echo "See docs/install.md (section \"Installing on KDE Linux (immutable)\")." >&2
  elif h=$(distro_hint); then
    echo "Install the missing packages with:" >&2
    echo "  $h" >&2
    echo "then run ./install.sh again." >&2
  else
    echo "Could not recognise your distribution. Install cmake, a C++17 compiler and the Qt 6 development" >&2
    echo "packages (Widgets and DBus), then run ./install.sh again. See docs/install.md." >&2
  fi
  exit 1
}

# Qt dev files are present if the CMake package files for Widgets and DBus can be found.
qt_cmake_found() {  # qt_cmake_found 6|5
  [ -n "$QT_DIR_HINT" ] && return 0
  for pkg in Widgets DBus; do
    f=
    for d in /usr/lib /usr/lib64 /usr/local/lib /usr/share /opt/qt*/lib /usr/lib/qt$1 /usr/lib/qt$1/lib /usr/lib64/qt$1; do
      [ -d "$d" ] || continue
      f=$(find "$d" -maxdepth 4 -name "Qt$1${pkg}Config.cmake" 2>/dev/null | head -n 1)
      [ -n "$f" ] && break
    done
    [ -n "$f" ] || return 1
  done
  return 0
}

if [ $mode = source ]; then
  QT_DIR_HINT=
  [ -n "$CMAKE_PREFIX_PATH" ] && QT_DIR_HINT=1
  miss=
  have cmake || { say "missing: cmake"; miss=1; }
  if ! have c++ && ! have g++ && ! have clang++; then say "missing: a C++17 compiler (g++ or clang++)"; miss=1; fi
  qt_cmake_found 6 || { say "missing: Qt 6 development files (Widgets, DBus)"; miss=1; }
  [ -z "$miss" ] || missing_help
  [ $system = 1 ] && [ -z "$P5" ] && qt5=0
  qt5_ok=0
  [ $qt5 = 1 ] && qt_cmake_found 5 && qt5_ok=1
  [ $qt5 = 1 ] && [ $qt5_ok = 0 ] && say "Qt 5 development files not found: building the Qt 6 plugin only (Qt 5 apps will not be styled)."
  bd=$here/build
  jobs=$(nproc 2>/dev/null || echo 2)
  [ -n "$JOBS" ] && jobs=$JOBS
  build() {  # build 6|5
    say "Building the Qt $1 plugin ..."
    cmake -S "$here" -B "$bd/install-qt$1" -DQT_MAJOR=$1 -DCMAKE_BUILD_TYPE=Release -DADWAITA_BUILD_GALLERY=OFF >"$bd/qt$1.log" 2>&1 \
      && cmake --build "$bd/install-qt$1" -j"$jobs" >>"$bd/qt$1.log" 2>&1
  }
  mkdir -p "$bd"
  if ! build 6; then
    tail -n 25 "$bd/qt6.log" >&2
    echo "Qt 6 build failed (full log: $bd/qt6.log). Missing development packages are the usual cause." >&2
    missing_help
  fi
  src6=$bd/install-qt6/qtlibadwaitastyle.so
  src5=
  if [ $qt5_ok = 1 ]; then
    if build 5; then src5=$bd/install-qt5/qtlibadwaitastyle.so
    else say "Qt 5 build failed (log: $bd/qt5.log); continuing with Qt 6 only."; fi
  fi
else
  [ $qt5 = 1 ] || src5=
fi

# ---- install files ----
# Replace files atomically (copy to a temp name, then rename). Running programs (KWin, plasmashell, every Qt app)
# have the old plugin and fonts memory-mapped; writing over them in place crashes those programs instantly.
put() {  # put SRC DIR
  dst="$2/$(basename "$1")"
  cmp -s "$1" "$dst" && return 0
  $SUDO cp "$1" "$dst.new.$$" && $SUDO mv -f "$dst.new.$$" "$dst"
}
if [ $system = 1 ]; then
  say "Installing system-wide (sudo): $P6"
  $SUDO mkdir -p "$P6" "$SCHEMES"
  [ -n "$src5" ] && [ -f "$src5" ] && $SUDO mkdir -p "$P5"
else
  mkdir -p "$P6" "$P5" "$SCHEMES"
fi
put "$src6" "$P6"
[ -n "$src5" ] && [ -f "$src5" ] && [ -n "$P5" ] && put "$src5" "$P5"
for f in "$schemes_src/"*.colors; do put "$f" "$SCHEMES"; done

# ---- Plasma theme parts: Global Themes, window decorations, Plasma Style, splash, icon themes ----
# Same atomic replacement as put(), but always as the user (these go to ~/.local/share even with --system).
put_user() {  # put_user SRC DIR
  dst="$2/$(basename "$1")"
  cmp -s "$1" "$dst" && return 0
  cp "$1" "$dst.new.$$" && mv -f "$dst.new.$$" "$dst"
}
put_tree() {  # put_tree SRC_DIR DST_DIR: every file below SRC_DIR, atomically
  (cd "$1" && find . -type f) | while IFS= read -r f; do
    mkdir -p "$2/$(dirname "$f")"; put_user "$1/$f" "$2/$(dirname "$f")"
  done
}
if [ $plasma = 1 ] && [ -d "$here/plasma" ]; then
  P=$here/plasma
  for id in $LNF_LIGHT $LNF_DARK; do put_tree "$P/look-and-feel/$id" "$L/share/plasma/look-and-feel/$id"; done
  put_tree "$P/desktoptheme/Libadwaita" "$L/share/plasma/desktoptheme/Libadwaita"
  for t in Libadwaita Libadwaita-Dark; do put_tree "$P/aurorae/$t" "$L/share/aurorae/themes/$t"; done
  say "Plasma: Global Themes Libadwaita and Libadwaita Dark, window decorations and Plasma Style installed"
  # Icon themes are built from the system's Adwaita icons (nothing is copied from this repository).
  icons_tool=$here/tools/make_plasma_icons.py
  if [ -f "$icons_tool" ] && have python3; then
    python3 "$icons_tool" --out "$L/share/icons" || say "Icon themes skipped (the Global Themes fall back to Breeze icons)."
  else
    say "Icon themes skipped: python3 or tools/make_plasma_icons.py not found."
  fi
fi

# Adwaita Sans / Mono (SIL OFL, see fonts/LICENSE); skipped if the system already has them.
fetch_fonts() {  # download the official tarball into a temp dir, then install the files
  t=$(mktemp -d) || return 1
  ok=
  if have curl; then curl -fsSL -o "$t/f.tar.xz" "$FONTS_URL" && ok=1
  elif have wget; then wget -q -O "$t/f.tar.xz" "$FONTS_URL" && ok=1; fi
  if [ -n "$ok" ] && have sha256sum && [ "$(sha256sum "$t/f.tar.xz" | cut -d' ' -f1)" != "$FONTS_SHA256" ]; then
    say "Font download failed its checksum; skipped."; rm -rf "$t"; return 0
  fi
  if [ -n "$ok" ] && have xz && tar -xJf "$t/f.tar.xz" -C "$t" 2>/dev/null; then
    mkdir -p "$fontdir"
    for f in "$t"/adwaita-fonts-*/sans/*.ttf "$t"/adwaita-fonts-*/mono/*.ttf "$t"/adwaita-fonts-*/LICENSE; do
      [ -f "$f" ] && put "$f" "$fontdir"
    done
    fc-cache -f "$L/share/fonts" >/dev/null 2>&1 || true
    rm -rf "$t"; return 0
  fi
  rm -rf "$t"; return 1
}
if [ $fonts = 1 ] && ! fc-list 2>/dev/null | grep -q "Adwaita Sans"; then
  if [ -d "$here/fonts" ]; then
    mkdir -p "$fontdir"
    for f in "$here/fonts/"*; do put "$f" "$fontdir"; done
    fc-cache -f "$L/share/fonts" >/dev/null 2>&1 || true
  else
    say "Downloading Adwaita fonts $FONTS_VERSION from download.gnome.org ..."
    fetch_fonts || say "Could not download the fonts (skipped; install adwaita-fonts from your distribution, or use --no-fonts)."
  fi
fi

# ---- environment ----
if [ $system = 0 ]; then
  mkdir -p "$(dirname "$envfile")" "$(dirname "$envd")"
  # Qt must find the plugin in ~/.local, also for Plasma's "Application Style" list.
  cat > "$envfile" <<EOT
export QT_PLUGIN_PATH=\$HOME/.local/lib/qt6/plugins:\$HOME/.local/lib/qt5/plugins\${QT_PLUGIN_PATH:+:\$QT_PLUGIN_PATH}
# Plasma's own QML shell needs this from the session (the style sets it only inside Qt widget apps);
# without it bold Adwaita Sans is emboldened twice. See docs/configuration.md.
export QT_NO_SYNTHESIZED_BOLD=1
EOT
  printf 'QT_PLUGIN_PATH=%s/.local/lib/qt6/plugins:%s/.local/lib/qt5/plugins\nQT_NO_SYNTHESIZED_BOLD=1\n' "$HOME" "$HOME" > "$envd"
  if [ $enable = 1 ]; then
    echo 'export QT_STYLE_OVERRIDE=Libadwaita' >> "$envfile"
    echo 'QT_STYLE_OVERRIDE=Libadwaita' >> "$envd"
  fi
elif [ $enable = 1 ]; then
  mkdir -p "$(dirname "$envd")"
  printf 'QT_STYLE_OVERRIDE=Libadwaita\nQT_NO_SYNTHESIZED_BOLD=1\n' > "$envd"
fi

# Flatpak apps. Default route: let every Flatpak app read the plugin from ~/.local (read-only) through a user
# override; works with any KDE runtime 6.x. If flatpak/*.flatpak extension bundles are shipped, install those too.
if have flatpak && [ $system = 0 ]; then
  flatpak override --user --filesystem="$L/lib/qt6/plugins:ro" --env=QT_PLUGIN_PATH="$L/lib/qt6/plugins:/app/lib/plugins:/app/plugins"
  say "Flatpak apps can now load the style (override in $L/share/flatpak/overrides/global)"
  if ls "$here"/flatpak/*.flatpak >/dev/null 2>&1; then
    for b in "$here"/flatpak/*.flatpak; do
      br=$(basename "$b" .flatpak); br=${br##*-}
      if flatpak info --user org.kde.Platform//"$br" >/dev/null 2>&1 || flatpak info --system org.kde.Platform//"$br" >/dev/null 2>&1; then
        flatpak install --user -y --reinstall --bundle "$b" >/dev/null && say "flatpak extension for KDE runtime $br installed"
      fi
    done
  fi
  [ $enable = 1 ] && flatpak override --user --env=QT_STYLE_OVERRIDE=Libadwaita
fi

# ---- --plasma: switch to the light Global Theme and register the light/dark pair ----
if [ $plasma_apply = 1 ]; then
  if [ $plasma = 0 ]; then say "--plasma needs the Plasma parts: drop --no-plasma."
  elif have plasma-apply-lookandfeel && have kwriteconfig6; then
    # keys of kcms/lookandfeel (lookandfeelsettings.kcfg): [KDE] DefaultLightLookAndFeel, DefaultDarkLookAndFeel, AutomaticLookAndFeel
    kwriteconfig6 --file kdeglobals --group KDE --key DefaultLightLookAndFeel $LNF_LIGHT
    kwriteconfig6 --file kdeglobals --group KDE --key DefaultDarkLookAndFeel $LNF_DARK
    [ $plasma_auto = 1 ] && kwriteconfig6 --file kdeglobals --group KDE --key AutomaticLookAndFeel true
    plasma-apply-lookandfeel -a $LNF_LIGHT && say "Plasma: Global Theme Libadwaita applied."
  else
    say "--plasma: plasma-apply-lookandfeel / kwriteconfig6 not found (this is not a Plasma 6 session); skipped."
  fi
fi

say
say "Installed. Log out and back in, then:"
[ $plasma = 1 ] && say "  KDE (one click): System Settings > Colours & Themes > Global Theme > Libadwaita (or Libadwaita Dark)"
say "  KDE (by hand):   System Settings > Colours & Themes > Application Style: Libadwaita, and Colours: Libadwaita or Libadwaita Dark"
[ $enable = 1 ] || say "  Other desktops: run ./install.sh --enable, or set QT_STYLE_OVERRIDE=Libadwaita"
say "Try one app first without logging out:"
if [ $system = 1 ]; then
  say "  QT_STYLE_OVERRIDE=Libadwaita kate"
else
  say "  QT_PLUGIN_PATH=$L/lib/qt6/plugins QT_STYLE_OVERRIDE=Libadwaita kate"
fi
say "  flatpak run --env=QT_STYLE_OVERRIDE=Libadwaita org.kde.kate"
