#!/bin/sh
# Build the portable release tarball: qt-libadwaita-<version>-<arch>.tar.gz
#   tools/make_release.sh [OUTDIR]        (default OUTDIR: ./dist)
# Builds the Qt 6 plugin and, if Qt 5 dev files exist, the Qt 5 plugin; stages them with the colour schemes,
# fonts, install.sh, README.md and COPYING. Build on the oldest distro you want to support (glibc, Qt 6.4).
set -e
root=$(cd "$(dirname "$0")/.." && pwd)
out=${1:-$root/dist}
version=$(sed -n 's/^project(qt-libadwaita VERSION \([0-9.]*\).*/\1/p' "$root/CMakeLists.txt")
version=${RELEASE_VERSION:-$version}

arch=$(uname -m)
name=qt-libadwaita-$version-$arch
bd=$root/build/release
jobs=${JOBS:-$(nproc 2>/dev/null || echo 2)}
FONTS_VERSION=51.0
FONTS_URL=https://download.gnome.org/sources/adwaita-fonts/51/adwaita-fonts-$FONTS_VERSION.tar.xz
FONTS_SHA256=fa104ae2c1b96580d322f563ffe8b2dfca5296b0eca4971efd2d8011405123d2

mkdir -p "$bd"
stage=$out/$name
rm -rf "$stage"; mkdir -p "$stage/qt6" "$stage/color-schemes" "$stage/fonts"

for q in 6 5; do
  cmake -S "$root" -B "$bd/qt$q" -DQT_MAJOR=$q -DCMAKE_BUILD_TYPE=Release -DADWAITA_BUILD_GALLERY=OFF >"$bd/qt$q.log" 2>&1 || {
    if [ $q = 6 ]; then cat "$bd/qt6.log" >&2; echo "Qt 6 configure failed" >&2; exit 1; fi
    echo "Qt 5 dev files not found: building without the Qt 5 plugin"; continue; }
  cmake --build "$bd/qt$q" -j"$jobs" >>"$bd/qt$q.log" 2>&1 || { tail -n 30 "$bd/qt$q.log" >&2; exit 1; }
  mkdir -p "$stage/qt$q"
  cp "$bd/qt$q/qtlibadwaitastyle.so" "$stage/qt$q/"
  strip --strip-unneeded "$stage/qt$q/qtlibadwaitastyle.so" 2>/dev/null || true
done

cp "$root"/data/kde-colors/*.colors "$stage/color-schemes/"

t=$(mktemp -d)
curl -fsSL -o "$t/f.tar.xz" "$FONTS_URL"
[ "$(sha256sum "$t/f.tar.xz" | cut -d' ' -f1)" = "$FONTS_SHA256" ] || { echo "font checksum mismatch" >&2; exit 1; }
tar -xJf "$t/f.tar.xz" -C "$t"
cp "$t"/adwaita-fonts-*/sans/*.ttf "$t"/adwaita-fonts-*/mono/*.ttf "$t"/adwaita-fonts-*/LICENSE "$stage/fonts/"
rm -rf "$t"

cp "$root/install.sh" "$stage/install.sh"; chmod +x "$stage/install.sh"
cp "$root/README.md" "$root/COPYING" "$stage/"
mkdir -p "$stage/docs"; cp "$root/docs/install.md" "$stage/docs/"

tar -C "$out" -czf "$out/$name.tar.gz" "$name"
echo "$out/$name.tar.gz"
