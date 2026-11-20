# Design tokens

**libadwaita 1.10.0 / GNOME 51**

Human-readable twin of `data/adwaita-1.10.json` (the JSON is the machine source for generated C++ tables).
All paths are relative to the libadwaita 1.10.0 tag, `src/stylesheet/` unless noted; `file:line` points at the SCSS source.
Units: CSS px = Qt logical px. "cc N%" means `color-mix(in srgb, currentColor N%, transparent)` = the widget's own
foreground colour at alpha N. "RUNTIME" = not a constant in CSS, computed by libadwaita/GTK when running.
Not used as sources: adwaita-qt, GTK3 Adwaita.

## 1. Colour schemes

Light/dark follows portal `org.freedesktop.appearance color-scheme` (1 = dark). High contrast follows portal `contrast` == 1
(`adw-settings-impl-portal.c:169`) or gsettings `high-contrast`; it maps to CSS `prefers-contrast: more`.
**High contrast does not change any palette colour**; it changes opacities (section 3), adds 1px borders and raises ring alphas.

Source: `_colors.scss:1-125` (light then dark overrides). RGB(r g b / a%) is non-premultiplied. Fg colours are *translucent* in light.

| Token (CSS var) | Light | Dark | Line |
|---|---|---|---|
| window-bg / window-fg | #fafafb / rgba(0,0,6,.80) | #222226 / #ffffff | _colors.scss:30-31 |
| view-bg / view-fg | #ffffff / rgba(0,0,6,.80) | #1d1d20 / #ffffff | :34-35 |
| headerbar-bg / fg | #ffffff / rgba(0,0,6,.80) | #2e2e32 / #ffffff | :39-40 |
| headerbar-border-color | rgba(0,0,6,.80) | #ffffff | :41 (used at border-opacity) |
| headerbar-backdrop | #fafafb (= window-bg) | #222226 | :42 |
| headerbar-shade / darker-shade | rgba(0,0,6,.12) / rgba(0,0,6,.12) | rgba(0,0,6,.36) / rgba(0,0,12,.90) | :43-44 |
| sidebar-bg / fg | #ebebed / rgba(0,0,6,.80) | #2e2e32 / #fff | :48-49 |
| sidebar-backdrop | #f2f2f4 | #28282c | :50 |
| sidebar-shade / border | rgba(0,0,6,.07) / .07 | rgba(0,0,6,.25) / .36 | :51-52 |
| secondary-sidebar-bg | #f3f3f5 | #28282c | :54 |
| secondary-sidebar-backdrop | #f6f6fa | #252529 | :56 |
| secondary-sidebar-shade / border | .07 / .07 | .25 / .36 | :57-58 |
| card-bg / fg | #ffffff / rgba(0,0,6,.80) | rgba(255,255,255,.08) / #fff | :62-63 |
| card-shade | rgba(0,0,6,.07) | rgba(0,0,6,.36) | :64 |
| dialog-bg / fg | #fafafb / rgba(0,0,6,.80) | #36363a / #fff | :67-68 |
| popover-bg / fg | #ffffff / rgba(0,0,6,.80) | #36363a / #fff | :71-72 |
| popover-shade | rgba(0,0,6,.07) | rgba(0,0,6,.25) | :73 |
| thumbnail-bg / fg | #ffffff / rgba(0,0,6,.80) | #39393d / #fff | :76-77 |
| shade-color | rgba(0,0,6,.07) | rgba(0,0,6,.25) | :80 |
| scrollbar-outline-color | #ffffff | rgba(0,0,12,.95) | :81, :122 |
| active-toggle-bg / fg | #ffffff / rgba(0,0,6,.80) | rgba(255,255,255,.20) / #fff | _colors.scss:~160-215 (palette block) |
| overview-bg / fg | #f3f3f5 / rgba(0,0,6,.8) | #28282c / #fff | same block |
| destructive-bg / fg | #e01b24 (red_3) / #fff | #c01c28 (red_4) / #fff | :12-13, :86 |
| success-bg / fg | #2ec27e (green_4) / #fff | #26a269 (green_5) | :16-17, :87 |
| warning-bg / fg | #e5a50a (yellow_5) / rgba(0,0,0,.80) | #cd9309 / rgba(0,0,0,.80) | :20-21, :88 |
| error-bg / fg | #e01b24 / #fff | #c01c28 / #fff | :23-24, :89 |
| accent-bg / fg | one of the 9 accents (sec. 2) / #ffffff | same | `adw-style-manager.c:150-166` |

Standalone (text-usable) semantic colours (`--destructive-color` etc.), resolved with the algorithm in sec. 2.2:

| | Light | Dark |
|---|---|---|
| destructive-color / error-color | #c30000 | #ff938c |
| success-color | #007c3d | #78e9ab |
| warning-color | #905400 | #ffc252 |

Derived opaque value: `.opaque` button bg = mix(window-bg 85%, window-fg 15%) (`_buttons.scss:8`) = light #d4d4d6, dark #434347.
insensitive bg = mix(window-bg 60%, view-bg 40%) (`_compat-colors.scss:12`).

Base palette (`_palette.scss:1-50`): blue 99c1f1 62a0ea 3584e4 1c71d8 1a5fb4; green 8ff0a4 57e389 33d17a 2ec27e 26a269; yellow f9f06b f8e45c f6d32d f5c211 e5a50a;
orange ffbe6f ffa348 ff7800 e66100 c64600; red f66151 ed333b e01b24 c01c28 a51d2d; purple dc8add c061cb 9141ac 813d9c 613583;
brown cdab8f b5835a 986a44 865e3c 63452c; light ffffff f6f5f4 deddda c0bfbc 9a9996; dark 77767b 5e5c64 3d3846 241f31 000000.

### Overlay colours and alphas (`_colors.scss:244-320`)
| Name | Value |
|---|---|
| border-color | cc @ border-opacity (0.15; hc 0.50) |
| hover / active | cc 7% / 16% |
| selected / selected-hover / selected-active | cc 10% / 13% / 19% |
| view hover / active | cc 4% / 8% |
| view selected / hover / active | accent-bg @ 25% / 32% / 39% |
| trough / hover / active | cc 15 / 20 / 25% (hc 30 / 40 / 50%) |
| slider (scale/switch knob) | mix(white 80%, view-bg); hover/active/checked: #ffffff |
| osd fg / bg | rgba(255,255,255,.90) / rgba(0,0,0,.70) |
| toast bg / fg | #505053 / #ffffff |
| tooltip border | rgba(255,255,255,.10) |
| window outline | rgba(255,255,255,.07) (hc .30) |
| link / visited | accent-color / mix(accent-color 80%, view-fg) ; hover HSL(h, s*1.1, l*1.1) (`_links.scss:11`) |
| drop target | accent-bg |

## 2. Accent colours

### 2.1 The nine accents (`adw-accent-color.c`, `_colors.scss` palette block)
blue #3584e4, teal #2190a4, green #3a944a, yellow #c88800, orange #ed5b00, red #e62d42, pink #d56199, purple #9141ac, slate #6f8396.
`--accent-bg-color` = this value, `--accent-fg-color` = white (`adw-style-manager.c:160-161`). Default accent: blue.

Standalone `--accent-color` resolved (RUNTIME formula in 2.2):

| accent | light | dark |
|---|---|---|
| blue | #0461be | #81d0ff |
| teal | #007184 | #7bdff4 |
| green | #15772e | #8de698 |
| yellow | #905300 | #ffc057 |
| orange | #b62200 | #ff9c5b |
| red | #c00023 | #ff888c |
| pink | #a2326c | #ffa0d8 |
| purple | #8939a4 | #fba7ff |
| slate | #526678 | #bbd1e5 |

### 2.2 Standalone derivation (`_colors.scss:25,88`; implemented in `adw-color-utils.c`, `adw-accent-color.c: adw_rgba_to_standalone`)
CSS: `oklab(from <bg> min(l,0.5) a b)` (light), `oklab(from <bg> max(l,0.85) a b)` (dark). Same for destructive/success/warning/error.
1. sRGB 0-255 -> /255 -> EOTF `v>=0.04045 ? ((v+0.055)/1.055)^2.4 : v/12.92`.
2. Linear -> Oklab with Ottosson matrices (exact constants in JSON `accent.standalone_algorithm.oklab_matrices`).
3. `L = dark ? max(L,0.85) : min(L,0.5)`; `a`,`b` unchanged; alpha unchanged.
4. Oklab -> linear -> gamma `v>0.0031308 ? 1.055*v^(1/2.4)-0.055 : 12.92*v`; clamp each channel to [0,1].
Inside `.osd`, `toast` the dark rule is forced (`_common.scss:41`, `_misc.scss:44`); in `toggle-group.osd` checked the light rule is forced.

### 2.3 Portal accent-color -> accent (`adw-settings-impl-portal.c:125-141`, `adw-accent-color.c: nearest_from_rgba`)
Portal gives (r,g,b) doubles; any component <0 or >1 => blue. Otherwise convert to OKLCH (C = hypot(a,b), h = atan2(b,a) deg mod 360) and take the first match:
`C<0.04 slate; h>345 pink; h>280 purple; h>230 blue; h>175 teal; h>115 green; h>75.5 yellow; h>35 orange; h>10 red; else pink`.
Lightness is ignored; only one of the nine fixed accents is ever used (no arbitrary colours). gsettings fallback uses the enum names (`adw-settings.c:125-180`).

## 3. Opacities / globals (`_colors.scss:248-262, 318-330`)
| Token | Normal | High contrast |
|---|---|---|
| --border-opacity | 15% | 50% |
| --dim-opacity (`.dimmed`, placeholders, subtitles, accelerators, marks) | 55% | 90% |
| --disabled-opacity (whole-widget opacity) | 50% | 40% |
| focus ring alpha (accent-color) | 50% | 80% |
| dimmer-opacity (menu arrows, indicators, split separators) | 30% | 80% |
| strong-disabled (flat buttons, edit icons) | 30% | 30% |

## 4. Global geometry and motion
Radii (`_common.scss:1-12`, `_message-dialog.scss:1`): button 9, card 12, menu item 9, popover 15, dialog sheet 15, window 15, alert 18, switch 14, check 6, circular/pill 9999, frame 12, toast 150.
Spacing scale: 3 / 6 / 9 / 12 / 18 / 24 / 36 (6px grid; 3 half-step).
Transitions (`_common.scss:2-8`): ease-out-quad = cubic-bezier(.25,.46,.45,.94). Button: background+box-shadow 200ms. Focus: outline-color/width/offset 200ms. Backdrop: 200ms ease-out.
Scrollbar: all 200ms linear. Tabbar tab: 150ms ease-in-out. Row: 200ms. Expander arrow rotate: 200ms. Needs-attention dot: 150ms ease-in.
Animations off (`gtk-enable-animations=false` / gsettings `enable-animations`): libadwaita injects `* { transition: none; }` (`adw-style-manager.c:475-478`). Qt: skip all animations.

### Focus ring (`_drawing.scss:8-30`)
Solid outline 2px, colour accent-color @ 50% (hc 80%). Default inner: offset -2 (drawn inside). Outer (offset +1): suggested/opaque/osd buttons, switch. Cards, list rows, entry rows, card buttons: offset -1.
Scale: ring around the slider, offset 0. Notebook tab: -2. Toggle-group toggles / tabgrid: outer offset 0.
Idle state is a transparent 0-width outline 4px further in/out, animated 200ms. Shown only for keyboard focus (`:focus:focus-visible`); entries use `:focus-within` (always shown when focused, also by mouse).

## 5. Widgets
Heights shown as total = min-height + padding. "Disabled" = widget opacity per sec. 3 unless stated.

### Button (`widgets/_buttons.scss:17-110`)
min 16x24, padding 5/10 => 34px tall; radius 9; label weight bold (700). bg cc 10%, hover 15%, active 30%, checked 30% (hover 35, active 40).
Text-only: padding-lr 17. Image-only: min-w 24, padding-lr 5. Icon+text: padding-lr 9, spacing 4, label +2 lr. Arrow button: padding-lr 9, spacing 4.
HC: inset 1px border (border-color) on non-flat buttons. Disabled: opacity.
- flat (:234-280): transparent; hover cc 7, active 16, checked 10 (13/19); disabled-unchecked opacity 30%; HC border only on hover/active/checked.
- suggested (:155-228): bg accent-bg, fg accent-fg; hover overlay cc 10%; active overlay rgba(0,0,6,.20); checked overlay .15 (hover .05, active .30); focus ring outer.
- opaque (:213): bg mix(window-bg 85, window-fg 15), fg window-fg; overlays as suggested.
- destructive (:183-211): *not filled*: fg destructive-color (standalone), bg cc 15% (hover 20, active 35, checked 35/40/45). Filled only with suggested/opaque classes.
- pill (:322): padding 10/32, radius 9999. circular (:312): min 34x34, padding 0. osd (:112-153): min 32x32, bg rgba(0,0,0,.65), fg white .9; hover/active mix black with currentColor (15% / 25% of 65%).
- card button (:328): bg card-bg, padding 0, weight inherit, hover cc 4, active 8, focus offset -1.
- linked (`_linked.scss`): inner corners 0, neighbours overlap by 1px (margin -1).
- split button (:472-640): radius 9, separator margin tb 6, inner arrow button padding-lr 4.
- toggle group (`_toggle-group.scss`): container bg cc 10%, padding 3, radius 9; toggle radius 6, min 34x28, text padding-lr 8, bold; checked bg active-toggle-bg, shadow 0 1px 3px 1px rgba(0,0,6,.07), 0 2px 6px 2px rgba(0,0,6,.03).
- link button (`_links.scss`): accent-color, underline.

### Entry (`_entries.scss:1-78`)
min-height 34, padding-lr 9, radius 9, spacing 6, bg cc 10%, no border, caret currentColor. Focus ring when focus-within (accent 50%, 2px, -2). HC inset 1px border.
Icons inside: opacity 70% (hc 85%), hover 100%, active 80% (hc 90%), margin 6. Placeholder: dimmed. Disabled: opacity.
Progress under entry: 2px accent-bg line, margin-bottom 3. Flat entry: padding 2, transparent, radius 0.

### Spin button (`_spin-button.scss`)
Entry shell, padding 0. Horizontal: text min-w 28 padding 6; +/- buttons min 22x16 flat, left border 1px cc 10% (hc 50%), outer corners radius 9. Vertical: text min 30x30, buttons stacked with 1px border.

### Dropdown / combo (`_dropdowns.scss`)
A button (34px tall) with content spacing 6, 16px arrow icon; since 1.10 supports .flat/.raised. Popover contents as popover.menu with 6px top padding; search bar padding 6.

### Switch (`_switch.scss`)
Knob 20x20 (radius 50%), padding 3, radius 14 => 46x26 (size from GTK layout: 2*knob + 2*padding wide, RUNTIME in GTK C, not in SCSS). Trough cc 15 / hover 20 / active 25 (hc 30/40/50).
Checked: bg accent-bg, hover overlay cc 10, active rgba(0,0,6,.20). Knob: mix(white 80%, view-bg), white on hover/checked; shadow 0 2px 4px rgba(0,0,6,.20), none when disabled. Ring outer +1. In toolbars margin tb 4. OSD checked bg white 60%.

### Check / radio (`_checks.scss`)
Indicator 14 + padding 3 = 20x20. Check radius 6, radio 50%. Unchecked: inset 2px ring cc 15% (hover 20, hc: border-color 50% / hover 60%); active: fill cc 25 (hc 60), no ring.
Checked/indeterminate: fill accent-bg, glyph accent-fg (check.svg / bullet.svg / dash.svg in `assets/`), hover overlay cc 10, active rgba(0,0,6,.20). Disabled opacity.
Checkbutton: padding 3 (text variant 4), spacing 4, radius 9, ring around whole control. In menus: 14px, padding 2 (radio 1 + 1px border cc 30%, hc 80%), no fill. Selection-mode circles: padding 7, radius 100.

### Scale / slider (`_scale.scss`)
Trough 10px thick (min 10 + padding 12 => 34px control extent), radius 99, cc 15 (hover 20; hc 30/40 + inset 1px border). Highlight accent-bg, hover overlay cc 10.
Knob 20x20 round (margin -8), bg mix(white 80%, view-bg), shadow 0 0 0 1px rgba(0,0,6,.10) (hc .35), 0 2px 4px rgba(0,0,6,.20); hover white. Marks: 6px long, 6px gap, 1px, cc @ dim-opacity. Value label dimmed, margin 9. Fine-tune: trough grows 3px per side.
With marks on one side the knob is rotated 45 deg with one square corner. Ring around knob, offset 0.

### Progress bar (`_progress-bar.scss`) and level bar (`_level-bar.scss`)
Progress: thickness 8 (horizontal min length 150, vertical 80), radius 99, trough cc 15 (hc 30 + inset 1px border), fill accent-bg. Text: 83% size, dimmed, tabular. OSD variant 2px, no trough. Empty fraction hides fill.
Level bar: 8px, radius 99; low = warning-bg, normal/high = accent-bg, full = success-bg, empty = trough colour; discrete blocks min 26, gap 2.

### Scrollbar (`_scrolling.scss:1-127`)
Slider min length 40; slider thickness 8 (expanded/hover) or 3 (overlay idle); radius 99. Trough margin 9 along axis, 8 cross-axis expanded, 4 cross-axis overlay idle => bar occupies 24px (expanded) or 11px (idle) of cross axis.
Slider bg cc 20 / hover 40 / active 60 (hc 40/60/100). Overlay indicator: 1px outline of scrollbar-outline-color @ 60% (idle 35%; hc 100% / 60%) at offset -8; overlay hovering trough bg cc 10% (hc 25%).
Overlay hovering state is set by GTK when the pointer nears the bar and bars fade out after a GTK timeout (RUNTIME, not in CSS). Disabled: slider opacity 0. Fine-tune slider accent-color 60% (hc 100%). Transition all 200ms linear. In menus slider min-height 26.
Overshoot: two radial gradients cc 12% (3% thick) + cc 5% (50%). Undershoot: shade-color @ 75%, 4px gradient (hc: 1px border).

### Notebook (`_notebook.scss`)
Header border 1px border-color on the content side. Tab min 30x30, padding 3/12 (+4 on the side facing the content), weight normal, margins 4 between tabs. Hover bg cc 7. Selected indicator: inset 4px accent-bg bar on the content side (hover: border-color). Close button 20x20, cc 30% then currentColor on hover. Content bg view-bg. Focus ring offset -2 on the checked tab.

### Tab bar (AdwTabBar, `_tab-view.scss:1-182`)
bar bg headerbar-bg, bottom line inset 0 -1px headerbar-shade (hc: border); tabbox min-height 34, padding tb 6; tab min-height 26, padding 4, radius 9; hover cc 7, active 16, selected 10 (13 / 19). Close button 24x24 radius 99. Drag indicator 2px accent-color 50% (hc 100%). Transition 150ms.

### Header bar (`_header-bar.scss`)
min-height 47 (46 content + 1px bottom line; `.flat`/`.titlebar` 46; default-decoration 37/36), padding 6/7/7/7, box spacing 6, start/end margin 6. bg headerbar-bg, fg headerbar-fg.
Bottom line: `inset 0 -1px headerbar-shade`; side lines inset 1px headerbar-border-color @ border-opacity (hc: bottom line uses border color). Window shadow under titlebar: 0 1px + 0 2px 4px headerbar-shade @ 50%.
Backdrop (window unfocused): bg headerbar-backdrop, children opacity .5, 200ms.
Title: bold, padding-lr 12; subtitle: 83%, dimmed. Image/arrow buttons inside headerbar are flat; linked groups raised. Window controls: spacing 3, circle icon 16+8 = 24, bg cc 10 / 15 / 30.
Toolbar (`.toolbar`, `_toolbars.scss:99`): padding 6, spacing 6. Search bar padding 6/6/7/6, action bar padding 7/6/6/6, banner padding 6 bg mix(#7d7d83 30%, window-bg).

### Popover and menu (`_popovers.scss`, `_menus.scss`)
Popover contents: radius 15, padding 8, bg popover-bg; shadow 0 0 0 1px rgba(0,0,0,.05), 0 1px 5px 1px rgba(0,0,0,.09), 0 2px 14px 3px rgba(0,0,0,.05) (hc: first ring = border-color 50%).
Menu (popover.menu): min-width 120, contents padding 0, section padding 6. Item: min 40x32, padding-lr 12, radius 9, hover/selected cc 10%, active 19% (HC adds 1px inset border). Separator margin 6 0. Accelerator cc @ dim-opacity, 12px gap. Submenu arrow 16px @ 30% (hc 80%). Section title bold, padding 4/32.
List rows inside menus: same 32px/12/9; standalone lists: row margin-lr 6, padding 9/12. Menubar item padding 4/8, radius 9, bottom line inset border.

### Lists, rows, boxed lists (`_lists.scss`)
List: bg view-bg, row padding 2, separators 1px border-color. Row hover cc 4, active 8, selected accent 25 (hover 32, active 39).
Boxed list (`:520`): card bg, radius 12, card shadow (sec. below), row separator 1px card-shade-color (hc border-color), hover cc 3, active cc 8, focus offset -1, last row no separator. `boxed-list-separate`: each row a card, margin-bottom 12.
Action row: header min-height 50, margin-lr 12, title margin-tb 6, title/subtitle spacing 3, prefix/icon gap 6, subtitle 83% dimmed. Entry row header margin-lr 6, editable padding-lr 6. Spin row buttons 30x30 circular. Button row min-height 40. Expander row nested list bg card-shade @ 50%, arrow rotates 200ms, checked arrow accent-color. Section header padding 18/2/6/2, .heading.
Navigation sidebar rows (`_sidebars.scss:149-163`): min-height 36, padding-lr 8, margin 0/6/2/6, radius 9; hover cc 7, active 16, selected 10 (13/19). Sidebar pane bg sidebar-bg, divider inset 1px sidebar-border (hc border-color).

### Tooltip (`_tooltip.scss`)
bg rgba(0,0,6,.80), fg white, border 1px rgba(255,255,255,.10), padding 6/10, radius 9, no shadow.

### Frames, separators, cards, toast, dialogs (`_misc.scss`, `_window.scss`, `_dialogs.scss`, `_message-dialog.scss`)
Separator 1px border-color (spacer 12). Frame 1px border-color radius 12. Paned separator 1px (wide 5) with inset 1px border-color line.
Card: bg card-bg, radius 12, shadow 0 0 0 1px rgba(0,0,6,.03), 0 1px 3px 1px rgba(0,0,6,.07), 0 2px 6px 2px rgba(0,0,6,.03) (hc: ring = border-color 50%).
Toast: bg #505053, radius 150, padding 6, margin 12/12/24/12, shadow 0 1px 3px 1px rgba(0,0,6,.07), 0 2px 6px 2px rgba(0,0,6,.03).
Window CSD: radius 15, shadow 0 0 14px 5px rgba(0,0,0,.15), 0 0 5px 2px rgba(0,0,0,.10), 0 0 0 1px rgba(0,0,0,.05) (hc ring .80); outline 1px rgba(255,255,255,.07) inside (hc .30). Backdrop shadow shrinks. Maximized/tiled: radius 0, no shadow.
Dialog sheet: radius 15, shadow 0 0 14px 2px rgba(0,0,6,.03), 0 0 5px 2px rgba(0,0,6,.10), 0 0 0 1px rgba(0,0,0,.05); bg window-bg; dimmer = shade-color with alpha x2.
Alert dialog: radius 18, bg dialog-bg, button area padding 24, buttons padding 10/20 radius 12 min-height 24, spacing 12.
Status page: icon 128 (compact 96), margins 36/12 (compact 24/12), spacing 36 (compact 24), title .title-1 (compact .title-2).

### Selection and text views (`_views.scss`, `_common.scss:56-63`)
Text selection: accent-bg @ 30% when focused, view-fg @ 10% when not. View item selection: accent-bg @ 25%/32%/39%, radius 9. Rubberband: 1px accent-color, fill accent-color @ 20%. Flowbox/grid child padding 3, radius 9.
Disabled view: fg view-fg @ disabled-opacity, bg mix(window-bg 60%, view-bg).

## 6. Typography
Default fonts, gsettings-desktop-schemas 51.0 `org.gnome.desktop.interface` (verified): `font-name` = **Adwaita Sans 11**, `document-font-name` = Adwaita Sans 12 (main branch: 11), `monospace-font-name` = Adwaita Mono 11.
libadwaita's stylesheet sets no `font-family` for normal text (it uses gtk-font-name); fallbacks when no setting: Sans 10 / Monospace 10 (`adw-style-manager.c:50-59`). 11pt = 14.67px at 96 dpi.

| Class (`_labels.scss:35-90`) | Weight | Size | Line height |
|---|---|---|---|
| .title-1 | 800 | 181% | |
| .title-2 | 800 | 136% | |
| .title-3 | 700 | 136% | |
| .title-4 | 700 | 118% | |
| .heading | 700 | 100% | |
| .body | 400 | 100% | 140% |
| .caption-heading | 700 | 82% | |
| .caption | 400 | 82% | 140% |
| .document | document font | | 140% |
| .monospace | monospace font | | |
| .numeric | tabular-nums | | |
| smaller (subtitles, progress text) | | ~83% (Pango `smaller`) | |

Button label 700; header bar title 700 (subtitle smaller + dimmed); tab labels 400 (notebook and AdwTabBar); menu items 400, menu section titles 700; toggle group 700; list row title normal, subtitle smaller + dimmed (55%).

## 7. High contrast summary
Palette unchanged. Changes: border-opacity 15 -> 50%; dim 55 -> 90%; disabled 50 -> 40%; dimmer 30 -> 80%; focus ring 50 -> 80%; troughs 15/20/25 -> 30/40/50; scrollbar slider 20/40/60 -> 40/60/100, overlay outlines stronger; entry icon opacity 70 -> 85%; tab indicators opacity 50 -> 100%.
Added 1px inset borders (border-color at 50%): non-flat buttons, flat buttons on hover/active/checked, entries/spinbuttons, toggle group, scale/progress/levelbar troughs, window-control circles, menu items and list rows on hover/active/selected, tabbar tabs, flowbox/grid children, cards and popover/toast rings, window ring (black 80%). Source: all `@media (prefers-contrast: more)` blocks (154 matches in `stylesheet/`).
