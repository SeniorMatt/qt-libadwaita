# Guidelines checklist

**libadwaita 1.10.0 / GNOME 51.** One rule per line; fill "Status" later (ok / partial / todo / n/a + note).
HIG = https://developer.gnome.org/hig/ ; AdwDocs = https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/

| # | Rule | Source | Status |
|---|---|---|---|
| 1 | Spacing follows the 6px grid (3, 6, 9, 12, 18, 24, 36); control gaps 6, group gaps 12-18 | [HIG UI styling](https://developer.gnome.org/hig/guidelines/ui-styling.html) | |
| 2 | Corner radii: buttons/entries/menu items 9, cards/boxed lists 12, popovers/windows/dialogs 15, alerts 18 | [HIG UI styling](https://developer.gnome.org/hig/guidelines/ui-styling.html), tokens.md s.4 | |
| 3 | Default font Adwaita Sans 11 (Adwaita Mono for code); titles/headings via weight and size classes only | [HIG typography](https://developer.gnome.org/hig/guidelines/typography.html) | |
| 4 | Button labels bold; flat buttons in header bars and toolbars, raised only for standalone/linked actions | [HIG buttons](https://developer.gnome.org/hig/patterns/controls/buttons.html) | |
| 5 | Suggested (accent fill) at most one per view, for the main safe action; never next to many others | [HIG buttons](https://developer.gnome.org/hig/patterns/controls/buttons.html) | |
| 6 | Destructive style (red text, tinted bg) only for destructive actions; filled only with suggested/opaque | [HIG buttons](https://developer.gnome.org/hig/patterns/controls/buttons.html), [AdwDocs style classes](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/style-classes.html) | |
| 7 | Pill buttons for primary actions in empty states/dialogs; circular for round icon buttons | [AdwDocs style classes](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/style-classes.html) | |
| 8 | Header bar 47px (46+1px line), centred bold title, window controls right, flat icon buttons | [HIG header bars](https://developer.gnome.org/hig/patterns/containers/header-bars.html) | |
| 9 | Menus/popovers: 32px items, 12px side padding, 9px radius, separators for sections, accelerators dimmed | [HIG menus](https://developer.gnome.org/hig/patterns/controls/menus.html), [popovers](https://developer.gnome.org/hig/patterns/containers/popovers.html) | |
| 10 | Dialogs: alert dialog 18px radius, buttons padded 10/20, one suggested/destructive max | [HIG dialogs](https://developer.gnome.org/hig/patterns/feedback/dialogs.html) | |
| 11 | Boxed lists: card background, 12px radius, 1px row separators, no separator after last row | [HIG boxed lists](https://developer.gnome.org/hig/patterns/containers/boxed-lists.html), [AdwDocs](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/boxed-lists.html) | |
| 12 | Lists and columns: hover 4%, active 8%, selection accent 25% (not solid fill) | [HIG lists](https://developer.gnome.org/hig/patterns/containers/list-column-views.html) | |
| 13 | Switches (46x26) for immediate on/off settings; checkboxes/radios (20x20 hit box) for form choices | [HIG switches](https://developer.gnome.org/hig/patterns/controls/switches.html), [checkboxes](https://developer.gnome.org/hig/patterns/controls/checkboxes.html) | |
| 14 | Sliders: 10px trough, 20px round knob; marks only when values matter | [HIG sliders](https://developer.gnome.org/hig/patterns/controls/sliders.html) | |
| 15 | Entries 34px tall, tinted background without border; spin buttons keep +/- inside the field | [HIG text fields](https://developer.gnome.org/hig/patterns/controls/text-fields.html), [spin buttons](https://developer.gnome.org/hig/patterns/controls/spin-buttons.html) | |
| 16 | Dropdowns look like buttons with a trailing arrow; popover list with check marks | [HIG drop-downs](https://developer.gnome.org/hig/patterns/controls/drop-downs.html) | |
| 17 | Tabs/view switchers: selected tab tinted (10%), not a raised card; close button on hover | [HIG tabs](https://developer.gnome.org/hig/patterns/nav/tabs.html) | |
| 18 | Sidebars: 36px rows, 9px radius, sidebar background with 1px divider | [HIG sidebars](https://developer.gnome.org/hig/patterns/nav/sidebars.html) | |
| 19 | Progress bars 8px, rounded; spinners for unknown duration | [HIG progress bars](https://developer.gnome.org/hig/patterns/feedback/progress-bars.html) | |
| 20 | Tooltips short, dark rgba(0,0,6,.8) bubble, 9px radius | [HIG tooltips](https://developer.gnome.org/hig/patterns/feedback/tooltips.html) | |
| 21 | Focus ring (2px accent, 50%) only for keyboard focus (focus-visible); entries show it on any focus | [HIG keyboard](https://developer.gnome.org/hig/guidelines/keyboard.html), _drawing.scss:8 | |
| 22 | Everything reachable by keyboard: Tab order, arrow keys in lists, Esc closes popovers/dialogs, mnemonics | [HIG keyboard](https://developer.gnome.org/hig/guidelines/keyboard.html) | |
| 23 | Text contrast >= 4.5:1 (3:1 large/UI); do not rely on colour alone | [HIG accessibility](https://developer.gnome.org/hig/guidelines/accessibility.html) | |
| 24 | High contrast (portal contrast=1): 50% borders, 1px outlines on controls, stronger rings and dim/disabled opacities | [AdwDocs styles](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/styles-and-appearance.html), tokens.md s.7 | |
| 25 | Reduced motion: honour gtk-enable-animations / gsettings enable-animations, drop every transition | [HIG accessibility](https://developer.gnome.org/hig/guidelines/accessibility.html), adw-style-manager.c:475 | |
| 26 | Accent colours: nine accents from portal accent-color via nearest-hue mapping; default blue; accent-fg white | [AdwDocs styles](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/styles-and-appearance.html), tokens.md s.2 | |
| 27 | Standalone accent/semantic text colours use the oklab lightness clamp (L<=0.5 light, L>=0.85 dark) | [AdwDocs CSS variables](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/css-variables.html) | |
| 28 | Dark style from portal color-scheme (or app override), live switching, same layout as light | [AdwDocs styles](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/styles-and-appearance.html) | |
| 29 | Overlay scrollbars: thin (3px) idle bar that expands to 8px on hover and fades out; classic scrollbars only when forced | libadwaita _scrolling.scss:47-113 | |
| 30 | Disabled controls dimmed by opacity (50%), not recoloured | [AdwDocs CSS variables](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1.10/css-variables.html) | |
| 31 | Unfocused (backdrop) windows: header bar uses backdrop colour, content fades to 50% in the bar | _header-bar.scss:41 | |
| 32 | Touch/pointer: minimum 34px hit targets for buttons and rows (HIG 32+ for menu items) | [HIG pointer and touch](https://developer.gnome.org/hig/guidelines/pointer-touch.html) | |
| 33 | RTL: mirror paddings, arrows, sliders and tab order | [HIG keyboard](https://developer.gnome.org/hig/guidelines/keyboard.html) | |
| 34 | Icons: Adwaita symbolic icons recoloured to text colour, 16px in controls, 32px large | [HIG UI icons](https://developer.gnome.org/hig/guidelines/ui-icons.html) | |
