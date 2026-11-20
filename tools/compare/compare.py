#!/usr/bin/env python3
"""compare.py: libadwaita reference shots vs Qt gallery shots -> sheets + metrics + REPORT.md.

Usage: compare.py [--ref DIR] [--qt DIR] [--out DIR] [--schemes a,b] [--accents a,b] [--sections a,b]
Needs numpy + Pillow (run.sh provides a venv).

Method: each shot is cut into widget rectangles by XY-cut on the "not background" mask (galleries lay widgets out
differently, so no absolute coordinates are used). Text-only rectangles are dropped. The two ordered lists are aligned
with a small DP (cost = dominant fill colour + height); unaligned widgets are reported as unmatched, never invented.
"""
import argparse, json, math, os, sys
import numpy as np
from PIL import Image, ImageDraw

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SCHEMES = ["light", "dark", "hc-light", "hc-dark"]
ACCENTS = ["blue", "orange"]
# Qt section name -> reference section name (identical today; edit here if the galleries diverge).
SECTION_MAP = {}
# Sections where the width legitimately depends on text / layout: only heights are judged.
WIDTH_FREE = {"buttons", "entries", "spinbutton", "dropdown", "tabs", "headerbar", "menu", "menu-shadow", "tooltip",
              "boxedlist", "slider", "progress", "focusring", "focusring-entry"}
TOL_PX, TOL_DE = 2.0, 2.0
# Reference shot is the widget itself (tight popover / tooltip): the whole image counts as one widget; on the Qt side the
# largest component is taken with a stricter mask threshold so soft window shadows do not inflate the box.
WHOLE = {"menu", "menu-shadow", "tooltip"}

# ---------------------------------------------------------------- colour
def _lab(rgb):
    c = np.array(rgb, float) / 255.0
    c = np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)
    x = (0.4124564 * c[0] + 0.3575761 * c[1] + 0.1804375 * c[2]) / 0.95047
    y = 0.2126729 * c[0] + 0.7151522 * c[1] + 0.0721750 * c[2]
    z = (0.0193339 * c[0] + 0.1191920 * c[1] + 0.9503041 * c[2]) / 1.08883
    f = lambda t: t ** (1 / 3) if t > 216 / 24389 else (24389 / 27 * t + 16) / 116
    fx, fy, fz = f(x), f(y), f(z)
    return 116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)

def de2000(a, b):
    L1, a1, b1 = _lab(a); L2, a2, b2 = _lab(b)
    C1, C2 = math.hypot(a1, b1), math.hypot(a2, b2); Cm = (C1 + C2) / 2
    G = 0.5 * (1 - math.sqrt(Cm ** 7 / (Cm ** 7 + 25 ** 7)))
    a1p, a2p = a1 * (1 + G), a2 * (1 + G)
    C1p, C2p = math.hypot(a1p, b1), math.hypot(a2p, b2)
    h1 = math.degrees(math.atan2(b1, a1p)) % 360; h2 = math.degrees(math.atan2(b2, a2p)) % 360
    dL, dC = L2 - L1, C2p - C1p
    if C1p * C2p == 0: dh = 0
    else:
        dh = h2 - h1
        dh = dh - 360 if dh > 180 else dh + 360 if dh < -180 else dh
    dH = 2 * math.sqrt(C1p * C2p) * math.sin(math.radians(dh / 2))
    Lm, Cmp = (L1 + L2) / 2, (C1p + C2p) / 2
    if C1p * C2p == 0: hm = h1 + h2
    elif abs(h1 - h2) <= 180: hm = (h1 + h2) / 2
    else: hm = (h1 + h2 + 360) / 2 if h1 + h2 < 360 else (h1 + h2 - 360) / 2
    T = (1 - 0.17 * math.cos(math.radians(hm - 30)) + 0.24 * math.cos(math.radians(2 * hm))
         + 0.32 * math.cos(math.radians(3 * hm + 6)) - 0.20 * math.cos(math.radians(4 * hm - 63)))
    dth = 30 * math.exp(-(((hm - 275) / 25) ** 2))
    Rc = 2 * math.sqrt(Cmp ** 7 / (Cmp ** 7 + 25 ** 7))
    Sl = 1 + 0.015 * (Lm - 50) ** 2 / math.sqrt(20 + (Lm - 50) ** 2)
    Sc, Sh = 1 + 0.045 * Cmp, 1 + 0.015 * Cmp * T
    Rt = -math.sin(math.radians(2 * dth)) * Rc
    return math.sqrt((dL / Sl) ** 2 + (dC / Sc) ** 2 + (dH / Sh) ** 2 + Rt * (dC / Sc) * (dH / Sh))

def hexc(c): return "#%02x%02x%02x" % tuple(int(v) for v in c)

# ---------------------------------------------------------------- widget extraction
def load(path, win_bg=(255, 255, 255)):
    im = Image.open(path)
    if im.mode == "RGBA":  # flatten transparent margins (menu shadow area) onto the scheme's window colour
        im = Image.alpha_composite(Image.new("RGBA", im.size, tuple(win_bg) + (255,)), im)
    return im.convert("RGB")

def background(a):
    flat = a.reshape(-1, 3).astype(np.int32)
    key = flat[:, 0] * 65536 + flat[:, 1] * 256 + flat[:, 2]
    v, c = np.unique(key, return_counts=True)
    k = int(v[np.argmax(c)]); return np.array([k >> 16, (k >> 8) & 255, k & 255])

def _cuts(prof, gap):
    """list of (start, end) empty runs >= gap strictly inside the profile (profile is tight: starts and ends on True)."""
    cuts, run = [], 0
    for i, v in enumerate(prof):
        if not v: run += 1
        else:
            if run >= gap: cuts.append((i - run, i))
            run = 0
    return cuts

def xycut(mask, x0, y0, gap, horiz, out, fails=0):
    """recursive guillotine cut at empty gaps >= gap (rows first, then columns, alternating); leaves = (x, y, w, h)."""
    ys, xs = np.nonzero(mask)
    if len(ys) == 0: return
    mask = mask[ys.min():ys.max() + 1, xs.min():xs.max() + 1]; x0 += int(xs.min()); y0 += int(ys.min())
    cuts = _cuts(mask.any(axis=1 if horiz else 0), gap)
    if not cuts:
        if fails >= 1: out.append((x0, y0, mask.shape[1], mask.shape[0])); return
        xycut(mask, x0, y0, gap, not horiz, out, fails + 1); return
    pos = 0; n = mask.shape[0 if horiz else 1]
    for a, b in cuts + [(n, n)]:
        part = mask[pos:a, :] if horiz else mask[:, pos:a]
        xycut(part, x0 + (0 if horiz else pos), y0 + (pos if horiz else 0), gap, not horiz, out)
        pos = b

def widgets(img, scale, tol=4, whole=False):
    a = np.asarray(img).astype(np.int32)
    bg = background(a)
    if whole:
        return [dict(x=0, y=0, w=img.width, h=img.height, dom=tuple(int(v) for v in bg), solid=1.0)], bg
    mask = np.abs(a - bg).sum(axis=2) > tol
    leaves = []
    xycut(mask, 0, 0, 3 * scale, True, leaves)
    res = []
    for (x, y, w, h) in leaves:
        m = mask[y:y + h, x:x + w]; px = a[y:y + h, x:x + w]
        ratio = m.mean()
        cols = px[m]
        key = cols[:, 0] * 65536 + cols[:, 1] * 256 + cols[:, 2]
        v, c = np.unique(key, return_counts=True); k = int(v[np.argmax(c)])
        dom = (k >> 16, (k >> 8) & 255, k & 255); solid = c.max() / (w * h)
        mn = min(w, h) / scale
        keep = (mn >= 14 and (solid >= 0.3 or mn >= 20)) or (ratio >= 0.7 and w * h / scale ** 2 >= 40)
        if not keep: continue
        res.append(dict(x=x, y=y, w=w, h=h, dom=dom, solid=float(solid)))
    return res, bg

def corner_radius(a, bg, wd, scale):
    """radius estimate (px at 1x) from the diagonal: first pixel off the background at d = r(1-1/sqrt2)."""
    x, y, w, h = wd["x"], wd["y"], wd["w"], wd["h"]
    best = []
    for cx, cy, sx, sy in ((x, y, 1, 1), (x + w - 1, y, -1, 1), (x, y + h - 1, 1, -1), (x + w - 1, y + h - 1, -1, -1)):
        i = 0
        while i < min(w, h) // 2 and np.abs(a[cy + sy * i, cx + sx * i] - bg).sum() < 12: i += 1
        best.append(i / 0.2929 / scale)
    r = float(np.median(best)); return round(min(r, min(w, h) / scale / 2), 1)

def ring_mode(a, wd, scale):
    """border colour: most common colour on the outermost pixel ring (corners excluded, 20 % each side)."""
    x, y, w, h = wd["x"], wd["y"], wd["w"], wd["h"]
    mx, my = max(int(w * .2), 1), max(int(h * .2), 1)
    px = np.concatenate([a[y, x + mx:x + w - mx], a[y + h - 1, x + mx:x + w - mx], a[y + my:y + h - my, x], a[y + my:y + h - my, x + w - 1]])
    if len(px) == 0: return tuple(a[y, x])
    key = px[:, 0] * 65536 + px[:, 1] * 256 + px[:, 2]; v, c = np.unique(key, return_counts=True); k = int(v[np.argmax(c)])
    return (k >> 16, (k >> 8) & 255, k & 255)

# ---------------------------------------------------------------- matching
def align(R, Q):
    """DP alignment of two ordered widget lists; returns list of (i, j) pairs."""
    def score(r, q):
        de = de2000(r["dom"], q["dom"]); hr = abs(math.log2(max(r["h"], 1) / max(q["h"], 1)))
        if de > 25 or hr > 0.8: return None
        return 20 - de * 0.6 - hr * 12
    n, m = len(R), len(Q); S = [[0.0] * (m + 1) for _ in range(n + 1)]; B = {}
    for i in range(n - 1, -1, -1):
        for j in range(m - 1, -1, -1):
            best, mv = S[i + 1][j], "r"
            if S[i][j + 1] > best: best, mv = S[i][j + 1], "q"
            sc = score(R[i], Q[j])
            if sc is not None and sc + S[i + 1][j + 1] > best: best, mv = sc + S[i + 1][j + 1], "m"
            S[i][j] = best; B[i, j] = mv
    i = j = 0; pairs = []
    while i < n and j < m:
        mv = B[i, j]
        if mv == "m": pairs.append((i, j)); i += 1; j += 1
        elif mv == "r": i += 1
        else: j += 1
    return pairs

# ---------------------------------------------------------------- one section
def compare_section(rp, qp, scale, section, win_bg):
    ri, qi = load(rp, win_bg), load(qp, win_bg)
    whole = section in WHOLE
    R, rbg = widgets(ri, scale, whole=whole); Q, qbg = widgets(qi, scale, tol=30 if whole else 4, whole=whole)  # gallery crops popups/tooltip tight, like the reference
    if whole and Q:
        Q = [max(Q, key=lambda w: w["w"] * w["h"])]
    ra, qa = np.asarray(ri).astype(np.int32), np.asarray(qi).astype(np.int32)
    if whole and Q: qbg = rbg  # the tight reference shot has no window background to compare
    out = dict(section=section, scale=scale, n_ref=len(R), n_qt=len(Q), bg_ref=hexc(rbg), bg_qt=hexc(qbg),
               bg_de=round(de2000(tuple(rbg), tuple(qbg)), 2), ref_size=ri.size, qt_size=qi.size, pairs=[])
    pairs = [(0, 0)] if whole and R and Q else align(R, Q) if R and Q else []
    for i, j in pairs:
        r, q = R[i], Q[j]
        re_, qe = ring_mode(ra, r, scale), ring_mode(qa, q, scale)
        if whole: re_, qe = r["dom"], q["dom"]  # outer ring of a rounded whole-image box is only corner anti-aliasing
        out["pairs"].append(dict(
            i=i, j=j, ref=[r["x"], r["y"], r["w"], r["h"]], qt=[q["x"], q["y"], q["w"], q["h"]],
            dw=round((q["w"] - r["w"]) / scale, 2), dh=round((q["h"] - r["h"]) / scale, 2),
            ref_fill=hexc(r["dom"]), qt_fill=hexc(q["dom"]), de_fill=round(de2000(r["dom"], q["dom"]), 2),
            ref_edge=hexc(re_), qt_edge=hexc(qe), de_edge=round(de2000(re_, qe), 2),
            rad_ref=corner_radius(ra, rbg, r, scale), rad_qt=corner_radius(qa, qbg, q, scale)))
    out["unmatched_ref"] = [[R[i]["x"], R[i]["y"], R[i]["w"], R[i]["h"]] for i in range(len(R)) if i not in {p[0] for p in pairs}]
    out["unmatched_qt"] = [[Q[j]["x"], Q[j]["y"], Q[j]["w"], Q[j]["h"]] for j in range(len(Q)) if j not in {p[1] for p in pairs}]
    out["_R"], out["_Q"] = R, Q
    return out

def verdict(res, section):
    """status + worst numbers (px at 1x, dE)."""
    if res is None: return dict(status="NO-QT", dh=None, dw=None, de=None)
    p = res["pairs"]
    if not p: return dict(status="NO-MATCH", dh=None, dw=None, de=None)
    dh = max(abs(x["dh"]) for x in p); dw = max(abs(x["dw"]) for x in p)
    de = max(max(x["de_fill"], x["de_edge"]) for x in p); de = max(de, res["bg_de"])
    full = len(p) == res["n_ref"] == res["n_qt"]
    bad = dh > TOL_PX or de >= TOL_DE or (section not in WIDTH_FREE and dw > TOL_PX)
    if bad: st = "FAIL"
    elif not full: st = "PARTIAL"  # numbers fine for matched widgets but some widgets could not be matched
    else: st = "OK"
    return dict(status=st, dh=dh, dw=dw, de=round(de, 2))

# ---------------------------------------------------------------- sheet
def heat(a):
    d = np.clip(a / 60.0, 0, 1)
    img = np.zeros(a.shape + (3,), np.uint8)
    img[..., 0] = (255 * np.clip(d * 2, 0, 1)); img[..., 1] = (255 * np.clip(d * 2 - 1, 0, 1)); img[..., 2] = (60 * (1 - d))
    return img

def make_sheet(rp, qp, res, path, win_bg):
    ri, qi = load(rp, win_bg), load(qp, win_bg)
    ra, qa = np.asarray(ri).astype(np.int32), np.asarray(qi).astype(np.int32)
    hm = np.zeros((ri.height, ri.width, 3), np.uint8) + 25
    R, Q = res["_R"], res["_Q"]
    matched_r = set()
    for p in res["pairs"]:
        i, j = p["i"], p["j"]; r, q = R[i], Q[j]; matched_r.add(i)
        w, h = max(r["w"], q["w"]), max(r["h"], q["h"])
        def crop(a, bgc, o):
            c = np.zeros((h, w, 3), np.int32) + np.array(bgc)
            hh, ww = min(h, a.shape[0] - o["y"]), min(w, a.shape[1] - o["x"])
            c[:hh, :ww] = a[o["y"]:o["y"] + hh, o["x"]:o["x"] + ww]
            return c
        d = np.abs(crop(ra, background(ra), r) - crop(qa, background(qa), q)).sum(axis=2)
        hh, ww = min(h, hm.shape[0] - r["y"]), min(w, hm.shape[1] - r["x"])
        hm[r["y"]:r["y"] + hh, r["x"]:r["x"] + ww] = heat(d)[:hh, :ww]
    im_h = Image.fromarray(hm); dr = ImageDraw.Draw(im_h)
    for i, r in enumerate(R):
        if i not in matched_r: dr.rectangle([r["x"], r["y"], r["x"] + r["w"] - 1, r["y"] + r["h"] - 1], outline=(255, 0, 255))
    H = max(ri.height, qi.height, 20) + 22
    sheet = Image.new("RGB", (ri.width + qi.width + im_h.width + 40, H), (255, 255, 255))
    d = ImageDraw.Draw(sheet)
    x = 10
    for lab, im in (("libadwaita", ri), ("Qt", qi), ("difference (matched widgets, magenta = unmatched ref)", im_h)):
        sheet.paste(im, (x, 18)); d.text((x, 4), lab, fill=(0, 0, 0)); d.rectangle([x - 1, 17, x + im.width, 18 + im.height], outline=(200, 0, 0))
        x += im.width + 10
    # widget boxes
    for lst, ox in ((R, 10), (Q, 10 + ri.width + 10)):
        for w in lst: d.rectangle([ox + w["x"], 18 + w["y"], ox + w["x"] + w["w"] - 1, 18 + w["y"] + w["h"] - 1], outline=(0, 160, 255))
    os.makedirs(os.path.dirname(path), exist_ok=True); sheet.save(path)

# ---------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", default=os.path.join(ROOT, "docs/reference")); ap.add_argument("--qt", default=os.path.join(ROOT, "docs/qt-shots"))
    ap.add_argument("--out", default=os.path.join(ROOT, "docs/compare"))
    ap.add_argument("--schemes", default=",".join(SCHEMES)); ap.add_argument("--accents", default=",".join(ACCENTS))
    ap.add_argument("--sections", default="")
    a = ap.parse_args()
    ref_sections = sorted({f.split("@")[0] for f in os.listdir(os.path.join(a.ref, "light/blue")) if f.endswith("@1x.png")})
    if a.sections: ref_sections = a.sections.split(",")
    inv = {v: k for k, v in SECTION_MAP.items()}
    allres = {}
    for scheme in a.schemes.split(","):
        for accent in a.accents.split(","):
            key = f"{scheme}-{accent}"; allres[key] = {}
            win_bg = tuple(Image.open(os.path.join(a.ref, scheme, accent, "buttons@1x.png")).convert("RGB").getpixel((1, 1)))
            for sec in ref_sections:
                qsec = inv.get(sec, sec); entry = {}
                for scale in (1, 2):
                    rp = os.path.join(a.ref, scheme, accent, f"{sec}@{scale}x.png")
                    qp = os.path.join(a.qt, scheme, accent, f"{qsec}@{scale}x.png")
                    if not os.path.exists(rp): continue
                    if not os.path.exists(qp): entry[scale] = None; continue
                    try:
                        res = compare_section(rp, qp, scale, sec, win_bg)
                    except Exception as e:  # report, do not invent numbers
                        print("ERROR", key, sec, scale, repr(e), file=sys.stderr); entry[scale] = None; continue
                    if scale == 1:
                        make_sheet(rp, qp, res, os.path.join(a.out, key, f"{sec}@1x.png"), win_bg)
                    for k in ("_R", "_Q"): res.pop(k, None)
                    entry[scale] = res
                allres[key][sec] = entry
    with open(os.path.join(a.out, "metrics.json"), "w") as f: json.dump(allres, f, indent=1)
    write_report(allres, a.out, ref_sections)

def widget_label(sec, p, res):
    return f"{sec} #{p['i'] + 1} (ref {p['ref'][2]}x{p['ref'][3]} at {p['ref'][0]},{p['ref'][1]})"

def write_report(allres, out, sections):
    L = ["# Libadwaita vs Qt: comparison report", "",
         "Generated by `tools/compare/run.sh` (`tools/compare/compare.py`). Reference = real libadwaita 1.10 shots (`docs/reference`),",
         "Qt = gallery shots (`docs/qt-shots`). Sheets: `<scheme>-<accent>/<section>@1x.png` = [libadwaita | Qt | difference].",
         "Sizes are px at 1x (2x measurements divided by 2). Widgets are matched in reading order by colour and height; width is",
         "judged only for fixed-width widgets (switches, checks, radios, scrollbars); other widths depend on text and are shown for info.",
         f"Status: OK = all widgets matched, |size diff| <= {TOL_PX:g} px and every dE2000 < {TOL_DE:g}. FAIL = a number is out of tolerance.",
         "PARTIAL = matched widgets are in tolerance but some widgets exist on one side only. NO-MATCH = automatic matching found nothing. NO-QT = no Qt shot.",
         "Worst dE covers main fill, border (outer pixel ring) and the window background colour.", ""]
    problems = {}
    summ = {}
    for key, secs in allres.items():
        L += [f"## {key}", "", "| section | widgets ref/Qt/matched | worst dH | worst dW | worst dE | bg dE | 2x dH / dE | status |", "|---|---|---|---|---|---|---|---|"]
        for sec in sections:
            e = secs.get(sec, {}); r1, r2 = e.get(1), e.get(2)
            v1, v2 = verdict(r1, sec), verdict(r2, sec)
            summ.setdefault(v1["status"], 0); summ[v1["status"]] += 1
            if r1 is None:
                L.append(f"| {sec} | - | - | - | - | - | - | {v1['status']} |"); continue
            f = lambda x: "-" if x is None else f"{x:g}"
            wcol = f(v1["dw"]) + ("" if sec not in WIDTH_FREE else " (info)")
            L.append(f"| {sec} | {r1['n_ref']}/{r1['n_qt']}/{len(r1['pairs'])} | {f(v1['dh'])} | {wcol} | {f(v1['de'])} | {r1['bg_de']} | {f(v2['dh'])} / {f(v2['de'])} | {v1['status']} |")
            # problems
            def add(kind, score, text):
                k = (sec, kind); cur = problems.get(k)
                if cur is None or score > cur[0]: problems[k] = [score, text, key, (cur[3] + 1) if cur else 1]
                else: cur[3] += 1
            if r1["bg_de"] >= TOL_DE: add("bg", r1["bg_de"] / TOL_DE, f"{sec}: window background dE {r1['bg_de']} ({r1['bg_ref']} ref vs {r1['bg_qt']} Qt)")
            if r1["n_ref"] != len(r1["pairs"]) and r1["pairs"]:
                add("unmatched", 1.0 + 0.1 * (r1["n_ref"] - len(r1["pairs"])), f"{sec}: {r1['n_ref'] - len(r1['pairs'])} of {r1['n_ref']} reference widgets have no Qt counterpart (Qt has {r1['n_qt']})")
            if not r1["pairs"]: add("nomatch", 2.0, f"{sec}: automatic matching failed (ref {r1['n_ref']} widgets, Qt {r1['n_qt']})")
            for p in r1["pairs"]:
                lab = widget_label(sec, p, r1)
                if abs(p["dh"]) > TOL_PX:
                    add(f"h{p['i']}", abs(p["dh"]) / TOL_PX, f"{lab}: Qt height {p['qt'][3]} vs ref {p['ref'][3]} ({p['dh']:+g} px)")
                if sec not in WIDTH_FREE and abs(p["dw"]) > TOL_PX:
                    add(f"w{p['i']}", abs(p["dw"]) / TOL_PX, f"{lab}: Qt width {p['qt'][2]} vs ref {p['ref'][2]} ({p['dw']:+g} px)")
                for nm, de, rc, qc in (("main fill", p["de_fill"], p["ref_fill"], p["qt_fill"]), ("border/edge", p["de_edge"], p["ref_edge"], p["qt_edge"])):
                    if de >= TOL_DE:
                        add(f"{nm}{p['i']}", de / TOL_DE, f"{lab}: {nm} colour dE {de} (libadwaita {rc}, Qt {qc})")
                if abs(p["rad_ref"] - p["rad_qt"]) > 3:
                    add(f"r{p['i']}", abs(p["rad_ref"] - p["rad_qt"]) / 6, f"{lab}: corner radius est. Qt {p['rad_qt']} vs ref {p['rad_ref']} (rough estimate)")
        L.append("")
    L += ["## Summary (1x, all scheme/accent tables)", "", ", ".join(f"{k}: {v}" for k, v in sorted(summ.items())), "",
          "## Top 20 problems", "", "Worst case across schemes/accents shown, with the scheme it occurs in and how many scheme/accent variants show the same problem. Radius values are rough diagonal estimates.", ""]
    top = sorted(problems.values(), key=lambda t: -t[0])[:20]
    for n, (score, text, key, cnt) in enumerate(top, 1):
        L.append(f"{n}. {text} [worst in {key}; {cnt} of {len(allres)} variants]")
    L.append("")
    with open(os.path.join(out, "REPORT.md"), "w") as f: f.write("\n".join(L))
    print("\n".join(L[-24:]))

if __name__ == "__main__": main()
