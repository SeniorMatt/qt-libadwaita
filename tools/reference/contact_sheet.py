#!/usr/bin/env python3
"""contact_sheet.py DIR OUT.png TITLE : tile every DIR/*@1x.png into one sheet (pycairo, runs in the Flatpak runtime)."""
import sys, glob, os, cairo
d, out, title = sys.argv[1:4]
files = sorted(glob.glob(os.path.join(d, "*@1x.png")))
imgs = [(os.path.basename(f)[:-7], cairo.ImageSurface.create_from_png(f)) for f in files]
COLW = 700; PAD = 10; LAB = 18
cols = [0, 0]; items = []
for n, s in imgs:
    c = 0 if cols[0] <= cols[1] else 1
    items.append((n, s, c, cols[c])); cols[c] += s.get_height() + LAB + PAD
H = max(cols) + 40
surf = cairo.ImageSurface(cairo.FORMAT_RGB24, 2 * COLW + 3 * PAD, H)
cr = cairo.Context(surf); cr.set_source_rgb(0.5, 0.5, 0.5); cr.paint()
cr.select_font_face("Sans"); cr.set_font_size(13)
cr.set_source_rgb(1, 1, 1); cr.move_to(PAD, 20); cr.show_text("libadwaita 1.10.0 reference - " + title + " (1x)")
for n, s, c, y in items:
    x = PAD + c * (COLW + PAD); y += 40
    cr.set_source_rgb(1, 1, 0.6); cr.move_to(x, y + 12); cr.show_text(n)
    cr.set_source_surface(s, x, y + LAB); cr.paint()
surf.write_to_png(out)
