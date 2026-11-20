# Configuration

How to change the look, and how apps can ask for GTK-style variants. Installing: [install.md](install.md).

## Desktop settings (automatic)
Dark/light, accent colour (nine accents), high contrast and reduced motion follow the desktop live, through
xdg-desktop-portal. On Plasma, dark and high contrast follow the KDE colour scheme instead; the portal gives the accent.

## Environment variables
| Variable | Values | Effect |
|---|---|---|
| `ADWAITA_COLOR_SCHEME` | `light` / `dark` | force the scheme |
| `ADWAITA_ACCENT` | `blue teal green yellow orange red pink purple slate` or `#rrggbb` | force the accent |
| `ADWAITA_HIGH_CONTRAST` | `0` / `1` | force high contrast |
| `ADWAITA_ANIMATIONS` | `0` / `1` | force animations on or off |
| `ADWAITA_SPACING` | `compact normal comfortable` or 0.5-3 | see below |
| `ADWAITA_FONT_SIZE` | points, `0` = app font | see below |
| `ADWAITA_KEEP_APP_PALETTE` / `ADWAITA_KEEP_APP_FONT` | `1` | keep the app's own palette / font |
| `ADWAITA_SYNTHETIC_BOLD` | `1` | keep Qt's synthetic bold. By default the style sets `QT_NO_SYNTHESIZED_BOLD=1`, because Qt otherwise emboldens the Bold instance of the variable Adwaita Sans a second time (bold text ~33% heavier than in GTK). The setting is process-wide: it stays in effect if the app later switches to another style, so fonts without a real bold face then show bold text as regular |

## Spacing and font size
Create `~/.config/qt-libadwaita.conf` (restart the apps after a change):

    [General]
    spacing=comfortable   # compact | normal | comfortable | any number 0.5-3 (gaps, margins, list rows)
    font-size=11          # Adwaita Sans size; default 10 (fits Qt/KDE layouts), 11 = exact GNOME size, 0 = app font

Or use the env vars `ADWAITA_SPACING` and `ADWAITA_FONT_SIZE`. Flatpak apps can't read that file; use
`flatpak override --user --env=ADWAITA_SPACING=comfortable <app-id>` instead.

## App-facing API (dynamic properties, mirroring GTK style classes)
| Property | Value | Effect |
|---|---|---|
| `adwaita-class` | `suggested-action` | accent-filled button (a QPushButton that is `default` looks the same) |
| `adwaita-class` | `destructive-action` | red button |
| `adwaita-class` | `flat` | transparent button (also `QPushButton::setFlat`) |
| `adwaita-class` | `pill` | fully rounded button |
| `adwaita-class` | `headerbar` | plain QWidget painted as a header bar |
| `adwaita-class` | `boxed-list` / `boxed-row` | card frame / row (+ `adwaita-row-index` int: separator above rows > 0) |
| `adwaita-switch` | `true` on a QCheckBox | draws a switch |
| `adwaita-focus-visible` | `true` | force the focus ring (testing) |

Example: `button->setProperty("adwaita-class", "suggested-action");`
