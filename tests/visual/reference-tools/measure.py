#!/usr/bin/env python3
"""measure.py REFDIR : pixel measurements from the 1x shots of one scheme/accent (pycairo; run in the runtime)."""
import sys, os, cairo
D = sys.argv[1]
class Im:
    def __init__(s, n):
        s.s = cairo.ImageSurface.create_from_png(os.path.join(D, n + "@1x.png")); s.d = s.s.get_data()
        s.w, s.h, s.st = s.s.get_width(), s.s.get_height(), s.s.get_stride(); s.bg = s.p(2, 2)
    def p(s, x, y):
        o = y * s.st + x * 4; b, g, r, a = s.d[o:o+4]; return (r, g, b)
    def hexp(s, x, y): return "#%02x%02x%02x" % s.p(x, y)
    def diff(s, a, b): return sum(abs(i - j) for i, j in zip(a, b))
    def bbox(s, x0, y0, x1, y1, tol=6):
        xs, ys = [], []
        for y in range(y0, min(y1, s.h)):
            for x in range(x0, min(x1, s.w)):
                if s.diff(s.p(x, y), s.bg) > tol: xs.append(x); ys.append(y)
        return (min(xs), min(ys), max(xs) - min(xs) + 1, max(ys) - min(ys) + 1) if xs else None
    def radius(s, bx, by, tol=40):
        """corner radius estimate from the diagonal: first pixel differing from bg at d = r(1-1/sqrt2)."""
        i = 0
        while i < 40 and s.diff(s.p(bx + i, by + i), s.bg) < tol: i += 1
        return round(i / 0.2929, 1)
def line(k, v): print(f"| {k} | {v} |")
print("| quantity | value |\n|---|---|")
b = Im("buttons"); line("window background", b.hexp(2, 2))
bb = b.bbox(0, 0, 100, 56); line("normal button size WxH", f"{bb[2]}x{bb[3]}"); line("normal button fill", b.hexp(20, 12 + bb[3] // 2))
line("normal button corner radius (est)", b.radius(bb[0], bb[1]))
line("suggested button fill", b.hexp(120, 12 + bb[3] // 2)); line("destructive button fill", b.hexp(250, 12 + bb[3] // 2))
pb = b.bbox(0, 56, 200, 130); line("pill button row 2 height", pb[3])
e = Im("entries"); eb = e.bbox(0, 0, 175, 70); line("entry size WxH (160 requested)", f"{eb[2]}x{eb[3]}")
line("entry fill / corner radius (est)", f"{e.hexp(eb[0] + 80, eb[1] + 17)} / {e.radius(eb[0], eb[1])}")
sp = Im("spinbutton"); line("spinbutton height", sp.bbox(0, 0, 150, 70)[3]); dd = Im("dropdown"); line("dropdown height", dd.bbox(0, 0, 150, 70)[3])
sw = Im("switches"); sb = sw.bbox(0, 0, 70, 64); line("switch size WxH", f"{sb[2]}x{sb[3]}")
line("switch off fill", sw.hexp(sb[0] + 8, sb[1] + 13)); sb2 = sw.bbox(70, 0, 130, 64); line("switch on fill", sw.hexp(sb2[0] + 8, sb2[1] + 13))
ck = Im("checks"); cb = ck.bbox(0, 0, 40, 64); line("check box size WxH (incl. border)", f"{cb[2]}x{cb[3]}")
cb2 = ck.bbox(40, 0, 80, 64); line("checked fill", ck.hexp(cb2[0] + 3, cb2[1] + 10)); line("unchecked fill", ck.hexp(cb[0] + 3, cb[1] + 10))
ra = Im("radios"); rb = ra.bbox(0, 0, 40, 64); line("radio size WxH", f"{rb[2]}x{rb[3]}")
sl = Im("slider"); sbb = sl.bbox(0, 0, 280, 70)
cols = {x: (sl.bbox(x, 0, x + 1, 70) or (0, 0, 0, 0))[3] for x in range(sbb[0], sbb[0] + sbb[2])}
kh = max(cols.values()); kx = [x for x, h in cols.items() if h == kh]
line("slider trough height (leftmost column)", cols[sbb[0] + 2]); line("slider knob height", kh)
line("slider knob width (approx, columns at max height)", len(kx)); line("slider knob x centre / scale width", f"{(kx[0]+kx[-1])/2 - sbb[0]:.0f} / {sbb[2]}")
sc = Im("scrollbar"); line("scrollbar hovered: width (row y=100, x 228..251)", sum(1 for x in range(228, 252) if sc.diff(sc.p(x, 100), sc.bg) > 6))
st = Im("scrollbar-thin"); xs = [x for x in range(228, 252) if st.diff(st.p(x, 60), st.bg) > 4]; line("scrollbar after scroll (thin indicator) width", len(xs))
tx = [x for x in range(228, 252) if sc.diff(sc.p(x, 100), sc.bg) > 6]
line("scrollbar hovered trough colour (x mid)", sc.hexp(tx[len(tx)//2], 100) if tx else "-")
hb = Im("headerbar"); hbb = hb.bbox(0, 0, 560, 72, tol=3); line("headerbar bbox WxH (536 requested)", f"{hbb[2]}x{hbb[3]}"); line("headerbar fill", hb.hexp(100, 30))
bl = Im("boxedlist"); sep = [y for y in range(14, 250) if bl.diff(bl.p(300, y), bl.p(300, y - 1)) > 4]
line("boxed list colour changes at y (x=300)", sep); bbl = bl.bbox(0, 0, 560, 250, tol=3); line("boxed list bbox WxH", f"{bbl[2]}x{bbl[3]}")
line("boxed list fill", bl.hexp(300, 20)); line("boxed list corner radius (est)", bl.radius(bbl[0], bbl[1], tol=3))
tb = Im("tabs"); tbb = tb.bbox(0, 0, 560, 71, tol=3); line("tab bar bbox WxH", f"{tbb[2]}x{tbb[3]}"); line("tab bar fill", tb.hexp(500, 20))
for x in (60, 200, 340): line(f"tab bar pixel x={x} y=20", tb.hexp(x, 20))
fr = Im("focusring"); fb = fr.bbox(0, 0, 200, 70, tol=3); line("focused button bbox incl. ring WxH", f"{fb[2]}x{fb[3]}")
line("focus ring pixels left of button (x=7..15, mid y)", " ".join(fr.hexp(x, 12 + 17) for x in range(7, 16)))
fe = Im("focusring-entry"); line("focused entry: pixels above entry (y=7..15, x=250)", " ".join(fe.hexp(250, y) for y in range(7, 16)))
pg = Im("progress"); pgb = pg.bbox(0, 0, 400, 50, tol=3); line("progress bbox WxH", f"{pgb[2]}x{pgb[3]}"); line("progress fill", pg.hexp(pgb[0] + 20, pgb[1] + pgb[3]//2))
m = Im("menu"); line("menu popover size", f"{m.w}x{m.h}"); line("menu fill", m.hexp(60, 8))
t = Im("tooltip"); tbx = t.bbox(0, 0, 200, 90); line("tooltip bbox WxH (own surface incl. none)", f"{tbx[2]}x{tbx[3]}"); line("tooltip fill", t.hexp(tbx[0] + 3, tbx[1] + 10))
