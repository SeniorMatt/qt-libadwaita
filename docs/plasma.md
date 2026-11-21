# KDE Plasma: Global Themes

`./install.sh` installs two complete Plasma 6 Global Themes next to the style plugin: **Libadwaita** (light) and
**Libadwaita Dark**. Switching to one takes one click:

**System Settings > Colours & Themes > Global Theme > Libadwaita** (or *Libadwaita Dark*) > *Apply*.

Or from a terminal: `./install.sh --plasma` (applies the light theme), or
`plasma-apply-lookandfeel -a io.github.seniormatt.libadwaita` / `io.github.seniormatt.libadwaita-dark`.
Nothing is changed until you apply a theme. Your panels, widgets and wallpaper are never touched (the themes
contain no desktop layout), and lock screen, login screen and logout screen stay Breeze.

## What a Global Theme sets

| Part | Light | Dark |
|---|---|---|
| Colours | Libadwaita | Libadwaita Dark |
| Application style | Libadwaita | Libadwaita |
| Window decoration | Aurorae "Libadwaita" | Aurorae "Libadwaita Dark" |
| Icons | Libadwaita | Libadwaita-Dark |
| Cursors | Adwaita | Adwaita |
| Plasma Style | Libadwaita | Libadwaita |
| Splash screen | Libadwaita | Libadwaita Dark |
| Fonts | Adwaita Sans / Adwaita Mono (below) | same |
| Window buttons | close only, on the right (below) | same |

The Global Theme list in System Settings also lets you tick which of these parts to apply, and the individual
pages (Colours, Window Decorations, Icons, ...) list every part on its own, so you can mix them.

### Window decoration

The Aurorae themes imitate a libadwaita header bar: header bar colour (the same colour the style paints for a tool
bar directly under it, so a title bar and the first tool bar merge into one bar like in GNOME), a dimmer colour
when the window is inactive, a bold centred title, 24 px round window buttons (10 % of the text colour, 15 % on
hover, 30 % pressed), 15 px top corners and the libadwaita window shadow. Qt applications draw square client
areas, so the bottom corners of a window stay square.

Settings that Plasma keeps per user: *System Settings > Colours & Themes > Window Decorations* has *Border size*
(pick *No Borders* to get the GNOME look; the theme has no side borders by itself) and the button layout.

### Window buttons

GNOME's default is a single close button on the right, and that is what the Global Themes set. To add minimise and
maximise: *Window Decorations > Configure Titlebar Buttons...*, drag *Minimise* and *Maximise* to the right (or use
`kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key ButtonsOnRight IAX`; letters: `M` menu, `S` on all
desktops, `H` help, `I` minimise, `A` maximise, `X` close, `F` keep above, `B` keep below). The title is centred
for the default layout; with more buttons it moves slightly off centre, as in any Aurorae theme.

### Fonts

Look-and-feel packages can set fonts, so the Global Themes do: *General*, *Toolbar* and *Menu* Adwaita Sans 10,
*Small* Adwaita Sans 8, *Fixed width* Adwaita Mono 10, *Window title* Adwaita Sans Bold 10. These match the style's
default `font-size=10` (see [Configure](../README.md#configure)). `install.sh` installs the fonts. If you run
the style with `font-size=11` (exact GNOME size) set the Plasma fonts to 11 and 9 as in the
[Fonts table](../README.md#fonts). If you do not want fonts changed, untick *Fonts* in the Global Theme page
before pressing *Apply*.

### Icons

The Adwaita icon theme is mostly symbolic, and its symbolic SVGs carry a fixed dark grey, which would be
invisible in a dark colour scheme; KDE applications also ask for names without `-symbolic`.
`tools/make_plasma_icons.py` (run by `install.sh`) therefore builds **Libadwaita** and **Libadwaita-Dark** in
`~/.local/share/icons` from the Adwaita icons on your system:

- the symbolic icons are copied with KDE's recolouring hook added (`style id="current-color-scheme"`, class
  `ColorScheme-Text`), so they follow the colour scheme like Breeze icons do;
- plain names (`document-open`) are added as links where Adwaita has only the symbolic icon;
- Adwaita's full-colour icons (folders, devices, file types, apps) are not copied, the theme inherits them
  (`Inherits=Adwaita,breeze,hicolor`; the dark one uses `breeze-dark`), and so does everything Adwaita lacks.

No icon is drawn or shipped by this project; the result is about 1.5 MB for both themes. The Adwaita icons and the
Adwaita cursor theme come from your distribution (package `adwaita-icon-theme`; on some distributions
`adwaita-icon-theme-legacy` as well for old PNG names). If they are missing the installer says so and the icon part
is skipped (the Global Themes then fall back to Breeze icons); install the package and run `./install.sh` again.

### Plasma Style

*Libadwaita* styles dialogs and popups (15 px corners like a libadwaita popover and its soft shadow), tooltips
(9 px), panels and widget backgrounds. It uses the KDE colour-scheme classes, so one theme follows light and dark.
Anything it does not provide (icons for the system tray, analogue clock, ...) comes from the default Plasma
style. It is at *Colours & Themes > Plasma Style*.

### Splash screen

A flat window-colour screen with a libadwaita-style spinner that fades in and out.

## Automatic light and dark

Plasma 6 can switch the Global Theme with the time of day or when the machine is idle. `./install.sh --plasma`
registers *Libadwaita* as the light theme and *Libadwaita Dark* as the dark one
(`kdeglobals`, group `KDE`: `DefaultLightLookAndFeel`, `DefaultDarkLookAndFeel`); `--plasma-auto` also
switches the automation on (`AutomaticLookAndFeel=true`). Without the script: *System Settings > Colours & Themes >
Global Theme > Choose...* (the automatic switching options; wording differs between Plasma versions).

## Remove

`./install.sh --remove` deletes everything it installed, the Plasma parts included (and the light/dark pair
entries if they are ours). If you had applied a Libadwaita theme, pick another Global Theme first, or Plasma falls
back to Breeze at the next login.

## Updating

All colours, radii and shadows of the decoration, the Plasma Style and the splash are generated from the libadwaita
token file, nothing is typed by hand:

```sh
python3 tools/gen_plasma_theme.py          # regenerate plasma/aurorae, plasma/desktoptheme, the splash
python3 tools/gen_plasma_theme.py --check  # exit 1 if the committed files are out of date (CI runs this)
```

`docs/updating-libadwaita.md` describes switching to a new libadwaita version; run the generator afterwards.

## Not part of it

Desktop layout, wallpaper, lock screen, login screen (SDDM), logout screen, window switcher and sounds. Libadwaita
has no counterpart for them, and replacing the user's panels is not something a theme should do.
