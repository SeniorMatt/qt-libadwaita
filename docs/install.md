# Install

qt-libadwaita is a style plugin for Qt 6 (6.4 or newer) and Qt 5.15. It installs into your home folder: no root needed.

## Quick install

From source (needs git, cmake, a C++17 compiler and the Qt 6 development packages, see below):

```sh
git clone https://github.com/SeniorMatt/qt-libadwaita.git
cd qt-libadwaita
./install.sh --enable
```

The script checks what is installed, tells you the exact command for your distribution if something is missing,
builds the Qt 6 plugin (and the Qt 5 one if Qt 5 development files exist), and installs the plugin, the KDE colour
schemes and the Adwaita fonts. `--enable` also makes every Qt app use the style; without it you pick the style
yourself (see "Turn it on"). Run `./install.sh --help` for all options.

Without a compiler: download `qt-libadwaita-<version>-<arch>.tar.gz` from the
[Releases](https://github.com/SeniorMatt/qt-libadwaita/releases) page, then

```sh
tar xf qt-libadwaita-*.tar.gz && cd qt-libadwaita-*/ && ./install.sh --enable
```

Log out and back in afterwards. To try one app first: `QT_PLUGIN_PATH=~/.local/lib/qt6/plugins QT_STYLE_OVERRIDE=Libadwaita kate`.

Options: `--enable` (switch it on for every Qt app), `--system` (install into the distribution's Qt plugin
directory with sudo instead of `~/.local`), `--no-fonts`, `--no-qt5`, `--remove`, and for KDE Plasma `--plasma` (also
apply the light Global Theme), `--plasma-auto` (also switch on automatic light/dark), `--no-plasma`.

On KDE Plasma the installer also puts the Global Themes **Libadwaita** and **Libadwaita Dark** (with window
decorations, icons, Plasma style and splash) into `~/.local/share`; switch with *System Settings > Colours & Themes >
Global Theme > Libadwaita*. See [plasma.md](plasma.md). The icon themes are generated from the Adwaita icons of your
distribution (package `adwaita-icon-theme`, needs python3).

## Dependencies per distribution

| Distribution | Command |
|---|---|
| Debian, Ubuntu, Mint, Pop!_OS | `sudo apt install build-essential cmake qt6-base-dev` (Qt 5 too: `qtbase5-dev`) |
| Fedora, RHEL and clones | `sudo dnf install gcc-c++ cmake qt6-qtbase-devel` (Qt 5 too: `qt5-qtbase-devel`) |
| Arch, Manjaro, EndeavourOS | `sudo pacman -S --needed base-devel cmake qt6-base` (Qt 5 too: `qt5-base`) |
| openSUSE | `sudo zypper install gcc-c++ cmake qt6-base-devel` (Qt 5 too: `libqt5-qtbase-devel`) |
| Void | `sudo xbps-install -S base-devel cmake qt6-base-devel` (Qt 5 too: `qt5-devel`) |
| Alpine | `sudo apk add build-base cmake qt6-qtbase-dev` (Qt 5 too: `qt5-qtbase-dev`) |
| KDE Linux and other immutable systems | see the last section |

Packaging recipes for Arch (`packaging/arch/PKGBUILD`) and Fedora (`packaging/rpm/qt-libadwaita.spec`) are in the repository.

## Turn it on

The style key is **Libadwaita** (the key "Adwaita" belongs to the old adwaita-qt).

- KDE Plasma: System Settings > Colours & Themes > Application Style: Libadwaita, and Colours: Libadwaita or
  Libadwaita Dark (KDE apps paint their content from the colour scheme). The installer already put both in place.
- GNOME and other desktops: run `./install.sh --enable`, or put `QT_STYLE_OVERRIDE=Libadwaita` into
  `~/.config/environment.d/qt.conf`, or start single apps with `QT_STYLE_OVERRIDE=Libadwaita myapp`.
- qt6ct / qt5ct: set Style to `Libadwaita` (run apps with `QT_QPA_PLATFORMTHEME=qt6ct`).
- Flatpak apps: the installer sets a user override so apps can read the plugin from `~/.local`
  (`flatpak override --user --filesystem=~/.local/lib/qt6/plugins:ro` and `QT_PLUGIN_PATH`); `--enable` adds
  `QT_STYLE_OVERRIDE=Libadwaita`. Works for apps on the KDE runtime 6.x.
- Wayland title bars (GNOME and others): install [QAdwaitaDecorations](https://github.com/FedoraQt/QAdwaitaDecorations)
  and run with `QT_WAYLAND_DECORATION=adwaita`. On Plasma, KWin draws the title bars.

Settings (dark/light, accent, contrast) and testing overrides: see [configuration.md](configuration.md).
Which desktop font to set to what: see the Fonts section of the [README](../README.md#fonts).

## Update

In your clone: `git pull && ./install.sh`. Files are replaced atomically, so running apps keep working; restart
them (or log out and back in) to see the new version. With a tarball: unpack the new one and run its `install.sh`.

## Uninstall

`./install.sh --remove` (also `--remove --system` if you used `--system`). It removes the plugins, colour schemes,
fonts it installed and the environment and Flatpak override entries it added.

## Build by hand

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build -j4
cmake --install build
```
This installs `qtlibadwaitastyle.so` into `<prefix>/<libdir>/qt6/plugins/styles` and `adwaita-gallery` into
`<prefix>/bin`. Use `-DADWAITA_PLUGIN_DIR=lib/x86_64-linux-gnu/qt6/plugins/styles` for another layout relative to the
prefix, or an absolute `-DQT_PLUGIN_INSTALL_DIR=/app/lib/plugins/styles` (e.g. for a Flatpak extension; overrides
everything else). Qt 5: add `-DQT_MAJOR=5` and a separate build directory; the plugin goes to `lib/qt5/plugins/styles`
and the gallery is named `adwaita-gallery-qt5`. Qt does not search `~/.local` by default:
`export QT_PLUGIN_PATH=$HOME/.local/lib/qt6/plugins`.

Release tarballs are made with `tools/make_release.sh` (build it on the oldest distribution you want to support).

## Installing on KDE Linux (immutable)

KDE Linux: image-based, /usr is read-only, Plasma and the core KDE apps are part of the image, other apps come
from Flatpak. So there are three kinds of Qt apps, and each needs its own route.

### 1. Apps in the image (Plasma, Dolphin, Kate, System Settings) - user-level plugin, no root
- Install the plugin under ~/.local (lib/qt6/plugins/styles/), the same as on any distro.
- Make Qt find it before Plasma starts: ~/.config/plasma-workspace/env/qt-libadwaita.sh with
  `export QT_PLUGIN_PATH=$HOME/.local/lib/qt6/plugins:$QT_PLUGIN_PATH` (also ~/.config/environment.d/ for
  non-Plasma sessions). Then the style shows up in System Settings > Colours & Themes > Application Style
  (written to kdeglobals [KDE] widgetStyle), no QT_STYLE_OVERRIDE needed.
- Building: the host has no compiler. Build in a container that matches the image's Qt (distrobox/toolbox with
  Arch, which KDE Linux is built from), or in the org.kde.Sdk Flatpak of the same Qt minor version.
- Qt version rule: a plugin built against Qt 6.x loads in Qt 6.y for y >= x, never the other way. Build against
  the Qt in the image (or older) and rebuild only if a major image update breaks it. The style therefore uses
  public Qt API only (no private headers) so it keeps working across image updates.

### 2. Flatpak apps on the KDE runtime (OBS, Kdenlive, qBittorrent, most Qt apps) - Flatpak extension
- Qt apps on org.kde.Platform load styles from the extension point org.kde.KStyle.<Name>, one build per
  runtime branch (currently 6.10 and 6.11). Flathub already has org.kde.KStyle.Adwaita, which is the old
  adwaita-qt, so ours needs its own name.
- Enable per user: `flatpak override --user --env=QT_STYLE_OVERRIDE=<key>` (all apps, or per app).
- Before publishing on Flathub, the extension can be built locally with flatpak-builder --user and installed
  --user (no root).
- Flatpak apps don't see host gsettings, but they do see xdg-desktop-portal, so reading accent, light/dark and
  contrast through the portal (our design) works inside the sandbox too.

### 3. Apps that bundle their own Qt (Telegram Desktop, some AppImages)
- Telegram Desktop links Qt statically and ignores style plugins. AppImages with their own Qt load plugins only
  if they were built with the same Qt minor version or older. Best effort only; document it.

### Optional: systemd-sysext (needs root once)
- A sysext image in /var/lib/extensions/ overlays /usr and puts the plugin in the real Qt plugin directory, so
  every host app finds it without QT_PLUGIN_PATH. Must be rebuilt whenever the image's Qt changes. Only worth
  it if route 1 proves unreliable.

### Settings source on Plasma
- xdg-desktop-portal-kde provides org.freedesktop.appearance color-scheme and accent-color (from the KDE colour
  scheme), so light/dark and the accent follow Plasma settings. Check whether it provides `contrast`; if not,
  fall back to the env override / a config key.
- Window decorations: on Plasma, KWin draws them (server-side), so QAdwaitaDecorations is not used there. An
  Adwaita-like KWin decoration (Aurorae theme or similar) would be a separate, optional item.
