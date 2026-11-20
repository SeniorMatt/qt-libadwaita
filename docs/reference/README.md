# libadwaita 1.10.0 reference shots

Real libadwaita widgets, rendered for pixel comparison with the Qt style.

## Versions (resolved at run time, see `<scheme>/<accent>/info.txt`)
- libadwaita 1.10.0, GTK 4.24.1, from Flatpak runtime `org.gnome.Platform//51` (PyGObject inside the runtime).
- Font: **Adwaita Sans 11** (`gtk-font-name`; the sandbox has no GSettings, so it is set explicitly to the GNOME 51 default; the
  runtime's `fc-match sans` would otherwise give DejaVu Sans). Hinting `hintslight`, antialias on, subpixel `none` (GNOME defaults).
- Icon theme Adwaita. Renderer: GSK **cairo** (deterministic, no GPU needed); GL may differ by sub-pixel rounding of shadows/gradients.

## How the shots were made
`tools/reference/shoot_all.sh` (whole matrix, skips finished variants; `FORCE=1` redoes all) starts a private Xvfb (no window
appears on the desktop, no host setting is touched) and runs `tools/reference/adw_gallery.py` in the Flatpak, 16 times:
scheme {light, dark, hc-light, hc-dark} x accent {blue, orange} x scale {1, 2}.
- Variants use libadwaita's debug env vars (verified: pixel colours and `info.txt` change): `ADW_DEBUG_COLOR_SCHEME=prefer-light|prefer-dark`,
  `ADW_DEBUG_HIGH_CONTRAST=0|1`, `ADW_DEBUG_ACCENT_COLOR=blue|orange` (also teal, green, yellow, red, pink, purple, slate). Scale 2 = `GDK_SCALE=2`.
- Each section is a fixed content box on the normal window background (`.background`), saved with `Gtk.WidgetPaintable` ->
  `GskRenderer.render_texture` -> `Gdk.Texture.save_to_png` at real device pixels (`@2x` files are 2x the pixels of `@1x`).
  No window decorations. `docs/reference/<scheme>/<accent>/<section>@<scale>x.png`.
- Sections: buttons, entries, spinbutton, dropdown, switches, checks, radios, slider, scrollbar, scrollbar-idle, scrollbar-thin, tabs,
  headerbar, menu, menu-shadow, boxedlist, tooltip, progress, focusring, focusring-entry. Disabled states are included in each widget section.
- scrollbar-idle: overlay scrollbar hidden. scrollbar-thin: thin indicator right after scrolling. scrollbar: hovered/expanded
  (pointer moved over it with `XWarpPointer`).
- focusring / focusring-entry: window `focus-visible` on, button resp. entry focused.
- menu: the real `GtkPopoverMenu` (4 items + separator) opened from a menu button; the popover widget is rendered tight. menu-shadow = same with
  24 px margin (no compositor under Xvfb, so no drop shadow is visible in it).
- Contact sheets: `sheet-<scheme>-<accent>.png` (1x), made by `tools/reference/contact_sheet.py` (pycairo, since PIL is not available).
- Measurements: `tools/reference/measure.py DIR` (pycairo pixel reads, runs in the runtime).

## Approximations / caveats
- tooltip: not captured from a real tooltip popup (GTK4 tooltip windows are not reachable as widgets). It is a widget with the same CSS node
  structure `tooltip.background > box > label`, so style is identical but the popup surface margins/shadow are missing.
- Menu popover has no drop shadow in the shot (see above); hover/pressed states of buttons are not captured (only scrollbar hover).
- Sections are measured on 1x; values are integers from pixel scans (edges anti-aliased, +-1 px; corner radius is an estimate from the
  diagonal, `d = r(1-1/sqrt2)`).

## Measured values (1x, from `light/blue` and `dark/blue`)
| quantity | light blue | dark blue |
|---|---|---|
| window background | #fafafb | #222226 |
| normal button size WxH | 82x34 | 82x34 |
| normal button fill | #e6e6e7 | #38383b |
| normal button corner radius (est) | 10.2 | 10.2 |
| suggested button fill | #3584e4 | #3584e4 |
| destructive button fill | #ebb4b4 | #604243 |
| pill button row 2 height | 44 | 44 |
| entry size WxH (160 requested) | 163x34 | 163x34 |
| entry fill / corner radius (est) | #a1a1a3 / 10.2 | #848486 / 10.2 |
| spinbutton height | 34 | 34 |
| dropdown height | 34 | 34 |
| switch size WxH | 46x27 | 46x26 |
| switch off fill | #ffffff | #d2d2d3 |
| switch on fill | #3584e4 | #3584e4 |
| check box size WxH (incl. border) | 20x20 | 20x20 |
| checked fill | #3584e4 | #3584e4 |
| unchecked fill | #fafafb | #222226 |
| radio size WxH | 20x20 | 20x20 |
| slider trough height (leftmost column) | 4 | 4 |
| slider knob height | 25 | 24 |
| slider knob width (approx, columns at max height) | 8 | 2 |
| slider knob x centre / scale width | 86 / 256 | 86 / 256 |
| scrollbar hovered: width (row y=100, x 228..251) | 8 | 8 |
| scrollbar after scroll (thin indicator) width | 3 | 5 |
| scrollbar hovered trough colour (x mid) | #e6e6e7 | #38383b |
| headerbar bbox WxH (536 requested) | 538x47 | 538x47 |
| headerbar fill | #ffffff | #2e2e32 |
| boxed list colour changes at y (x=300) | [66, 67, 121, 122, 176, 177, 231, 232, 233, 234, 235, 236] | [66, 67, 121, 122, 176, 177, 231, 232] |
| boxed list bbox WxH | 544x228 | 540x223 |
| boxed list fill | #ffffff | #333337 |
| boxed list corner radius (est) | 13.7 | 13.7 |
| tab bar bbox WxH | 480x47 | 480x47 |
| tab bar fill | #fafafb | #222226 |
| tab bar pixel x=60 y=20 | #ffffff | #2e2e32 |
| tab bar pixel x=200 y=20 | #ebebeb | #424246 |
| tab bar pixel x=340 y=20 | #ffffff | #2e2e32 |
| focused button bbox incl. ring WxH | 188x34 | 188x34 |
| focus ring pixels left of button (x=7..15, mid y) | #fafafb #fafafb #fafafb #fafafb #fafafb #75a3d2 #75a3d2 #e6e6e7 #e6e6e7 | #222226 #222226 #222226 #222226 #222226 #5c849d #5c849d #38383b #38383b |
| focused entry: pixels above entry (y=7..15, x=250) | #fafafb #fafafb #fafafb #fafafb #fafafb #75a3d2 #75a3d2 #e6e6e7 #e6e6e7 | #222226 #222226 #222226 #222226 #222226 #5c849d #5c849d #38383b #38383b |
| progress bbox WxH | 300x8 | 300x8 |
| progress fill | #3584e4 | #3584e4 |
| menu popover size | 128x165 | 128x165 |
| menu fill | #ffffff | #36363a |
| tooltip bbox WxH (own surface incl. none) | 58x18 | 58x18 |
| tooltip fill | #000004 | #000004 |

Notes: focus ring = 2 px inset ring in the accent colour at ~50 % alpha over the widget (blended pixel values shown), no outer offset for buttons/entries.
Switch knob/check borders are inside the stated sizes. Slider knob width is unreliable in this scan (read the `slider@2x` shot); trough is 4 px.
Boxed list row height = 55 px (separator 1 px) for rows with subtitle/entry title; window bg is not pure white (#fafafb light, #222226 dark).
