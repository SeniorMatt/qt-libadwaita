# Installing on KDE Linux (immutable) - plan

KDE Linux: image-based, /usr is read-only, Plasma and the core KDE apps are part of the image, other apps come
from Flatpak. So there are three kinds of Qt apps, and each needs its own route.

## 1. Apps in the image (Plasma, Dolphin, Kate, System Settings) - user-level plugin, no root
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

## 2. Flatpak apps on the KDE runtime (OBS, Kdenlive, qBittorrent, most Qt apps) - Flatpak extension
- Qt apps on org.kde.Platform load styles from the extension point org.kde.KStyle.<Name>, one build per
  runtime branch (currently 6.10 and 6.11). Flathub already has org.kde.KStyle.Adwaita, which is the old
  adwaita-qt, so ours needs its own name.
- Enable per user: `flatpak override --user --env=QT_STYLE_OVERRIDE=<key>` (all apps, or per app).
- Before publishing on Flathub, the extension can be built locally with flatpak-builder --user and installed
  --user (no root).
- Flatpak apps don't see host gsettings, but they do see xdg-desktop-portal, so reading accent, light/dark and
  contrast through the portal (our design) works inside the sandbox too.

## 3. Apps that bundle their own Qt (Telegram Desktop, some AppImages)
- Telegram Desktop links Qt statically and ignores style plugins. AppImages with their own Qt load plugins only
  if they were built with the same Qt minor version or older. Best effort only; document it.

## Optional: systemd-sysext (needs root once)
- A sysext image in /var/lib/extensions/ overlays /usr and puts the plugin in the real Qt plugin directory, so
  every host app finds it without QT_PLUGIN_PATH. Must be rebuilt whenever the image's Qt changes. Only worth
  it if route 1 proves unreliable.

## Settings source on Plasma
- xdg-desktop-portal-kde provides org.freedesktop.appearance color-scheme and accent-color (from the KDE colour
  scheme), so light/dark and the accent follow Plasma settings. Check whether it provides `contrast`; if not,
  fall back to the env override / a config key.
- Window decorations: on Plasma, KWin draws them (server-side), so QAdwaitaDecorations is not used there. An
  Adwaita-like KWin decoration (Aurorae theme or similar) would be a separate, optional item.
