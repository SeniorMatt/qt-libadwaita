#!/usr/bin/env python3
"""Generate KDE colour schemes (data/kde-colors/Libadwaita*.colors) from data/adwaita-1.10.json.
KDE apps (Dolphin, Kate, Okular...) paint many areas with KColorScheme from kdeglobals, not with the Qt palette,
so they need a matching KDE colour scheme next to the style."""
import json, os
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
d = json.load(open(os.path.join(root, "data/adwaita-1.10.json")))

def flat(c, over):
    a = c["a"]
    return tuple(round(c[k] * a + over[i] * (1 - a)) for i, k in enumerate("rgb"))
def rgb(t): return ",".join(str(v) for v in t)

for scheme, name, fname in [("light", "Libadwaita", "Libadwaita"), ("dark", "Libadwaita Dark", "LibadwaitaDark"),
                            ("hc-light", "Libadwaita High Contrast", "LibadwaitaHighContrast"),
                            ("hc-dark", "Libadwaita High Contrast Dark", "LibadwaitaHighContrastDark")]:
    c = d["schemes"][scheme]["colors"]
    dark = "dark" in scheme
    W = flat(c["window_bg"], (0, 0, 0)); V = flat(c["view_bg"], (0, 0, 0))
    def fg(key, bg): return flat(c[key], bg)
    def dim(key, bg, a): x = dict(c[key]); x["a"] = x["a"] * a; return flat(x, bg)
    acc_bg = flat(d["accent"]["palette"]["blue"], (0, 0, 0))
    hexa = d["accent"]["standalone_resolved_hex"]["dark" if dark else "light"]["blue"]
    acc = tuple(int(hexa[i:i + 2], 16) for i in (1, 3, 5))
    neg = (255, 136, 140) if dark else (192, 28, 40)
    neu = (248, 228, 92) if dark else (156, 110, 3)
    pos = (120, 233, 171) if dark else (27, 133, 83)
    shade = flat({"r": 0, "g": 0, "b": 6, "a": 0.07 if not dark else 0.25}, V)
    btn = flat({"r": c["window_fg"]["r"], "g": c["window_fg"]["g"], "b": c["window_fg"]["b"], "a": 0.1}, W)
    sections = {
        "View": (V, shade, fg("view_fg", V), V),
        "Window": (W, flat(c["sidebar_bg"], W), fg("window_fg", W), W),
        "Button": (btn, btn, fg("window_fg", W), W),
        "Header": (flat(c["headerbar_bg"], W), flat(c["headerbar_backdrop"], W), fg("headerbar_fg", W), W),
        "Tooltip": ((0x2e, 0x2e, 0x32) if not dark else (0xf6, 0xf5, 0xf4), (0x2e, 0x2e, 0x32) if not dark else (0xf6, 0xf5, 0xf4),
                    (255, 255, 255) if not dark else (0x24, 0x24, 0x24), W),
        "Complementary": ((0x22, 0x22, 0x26), (0x2e, 0x2e, 0x32), (255, 255, 255), (0x22, 0x22, 0x26)),
        "Selection": (acc_bg, acc_bg, (255, 255, 255), acc_bg),
    }
    out = [f"[General]\nColorScheme={fname}\nName={name}\nshadeSortColumn=false\n",
           "[KDE]\ncontrast=4\n",
           "[ColorEffects:Disabled]\nColor=56,56,56\nColorAmount=0\nColorEffect=0\nContrastAmount=0.5\nContrastEffect=1\n"
           "IntensityAmount=0\nIntensityEffect=0\n",
           "[ColorEffects:Inactive]\nChangeSelectionColor=false\nEnable=false\n"]
    for sec, (bg, alt, f, under) in sections.items():
        inactive = flat({"r": f[0], "g": f[1], "b": f[2], "a": 0.55}, bg)
        link = (255, 255, 255) if sec == "Selection" else acc
        out.append(f"[Colors:{sec}]\nBackgroundAlternate={rgb(alt)}\nBackgroundNormal={rgb(bg)}\n"
                   f"DecorationFocus={rgb(acc_bg)}\nDecorationHover={rgb(acc_bg)}\nForegroundActive={rgb(acc)}\n"
                   f"ForegroundInactive={rgb(inactive)}\nForegroundLink={rgb(link)}\nForegroundNegative={rgb(neg)}\n"
                   f"ForegroundNeutral={rgb(neu)}\nForegroundNormal={rgb(f)}\nForegroundPositive={rgb(pos)}\n"
                   f"ForegroundVisited={rgb(link)}\n")
    hb = flat(c["headerbar_bg"], W)
    out.append(f"[WM]\nactiveBackground={rgb(hb)}\nactiveBlend={rgb(hb)}\nactiveForeground={rgb(fg('headerbar_fg', W))}\n"
               f"inactiveBackground={rgb(flat(c['headerbar_backdrop'], W))}\ninactiveBlend={rgb(W)}\n"
               f"inactiveForeground={rgb(dim('headerbar_fg', W, 0.5))}\n")
    open(os.path.join(root, "data", "kde-colors", fname + ".colors"), "w").write("\n".join(out))
    print("wrote data/kde-colors/" + fname + ".colors")
