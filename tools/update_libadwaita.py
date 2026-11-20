#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Update the libadwaita design tokens from upstream.

  tools/update_libadwaita.py 1.10.0            # tag: download, extract, write data/adwaita-1.10.json, print the diff
  tools/update_libadwaita.py main --dry-run    # branch: only report, write nothing in the repo
  tools/update_libadwaita.py latest --apply    # newest stable tag, then also: data/current, generated header,
                                               # KDE schemes, build check, list of changed files
  tools/update_libadwaita.py --check           # current vs newest stable tag (key=value lines, for CI)

How it works
  1. Downloads src/stylesheet of the ref from gitlab.gnome.org (tag archive, a few 10 kB) into a cache directory.
  2. Compiles gtk.scss to CSS (libsass / sassc / sass; libsass is pip-installed if missing). The compiled CSS holds
     every @define-color, :root custom property and widget rule; `$variables` are evaluated by the same compile.
  3. Fills the JSON from that CSS (see EXTRACT below and SCHEME_COLORS etc.). The previous JSON is the template:
     every key that cannot be derived (hand-measured metrics, prose, line references, values that live in libadwaita's
     C code) is carried over unchanged and listed as "manual" in the output so a human checks it.
  4. Prints a readable diff old -> new. A selector that disappeared upstream keeps its old value and is warned about.
"""
import argparse, copy, glob, json, math, os, re, shutil, subprocess, sys, tarfile, tempfile
import urllib.error, urllib.parse, urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import tokensource  # noqa: E402

ROOT = tokensource.ROOT
DATA = tokensource.DATA
GITLAB = "https://gitlab.gnome.org"
PROJECT = "GNOME%2Flibadwaita"
STABLE_TAG = re.compile(r"^\d+\.\d+\.\d+(\.\d+)?$")
SCHEMES = ("light", "dark", "hc-light", "hc-dark")


# --------------------------------------------------------------------------------------------------- download
def http_get(url, binary=False):
    req = urllib.request.Request(url, headers={"User-Agent": "qt-libadwaita-token-updater"})
    try:
        with urllib.request.urlopen(req, timeout=60) as r:
            data = r.read()
    except urllib.error.HTTPError as e:
        raise SystemExit("download failed (%s): %s" % (e.code, url))
    except urllib.error.URLError as e:
        raise SystemExit("download failed (%s): %s" % (e.reason, url))
    return data if binary else data.decode("utf-8")


def vtuple(v):
    return tuple(int(x) for x in re.findall(r"\d+", v))


def latest_stable_tag():
    """Newest released tag (x.y.z or x.y.z.w; no alpha/beta/rc), by version number, not by date."""
    url = "%s/api/v4/projects/%s/repository/tags?per_page=100&order_by=version&sort=desc" % (GITLAB, PROJECT)
    tags = [t["name"] for t in json.loads(http_get(url))]
    stable = [t for t in tags if STABLE_TAG.match(t)]
    if not stable:
        raise SystemExit("no stable libadwaita tag found")
    return max(stable, key=vtuple)


def fetch_sources(ref, cache):
    """Return the directory holding the stylesheet sources of `ref` (src/stylesheet)."""
    if not re.match(r"^[A-Za-z0-9._/-]+$", ref) or ".." in ref:
        raise SystemExit("odd ref name: %r" % ref)
    safe = ref.replace("/", "_")
    dest = os.path.join(cache, "libadwaita-" + safe)
    is_tag = bool(re.match(r"^\d", ref))
    if os.path.isdir(os.path.join(dest, "stylesheet")) and is_tag:  # tags never change, branches do
        print("using cached sources: " + dest)
    else:
        shutil.rmtree(dest, ignore_errors=True)
        os.makedirs(dest)
        url = "%s/GNOME/libadwaita/-/archive/%s/libadwaita-%s.tar.gz?path=src/stylesheet" % (
            GITLAB, urllib.parse.quote(ref), safe)
        print("downloading " + url)
        tgz = os.path.join(dest, "src.tgz")
        with open(tgz, "wb") as f:
            f.write(http_get(url, binary=True))
        with tarfile.open(tgz) as t:
            for m in t.getmembers():  # data only, nothing outside dest
                if not (m.isreg() or m.isdir()) or m.name.startswith("/") or ".." in m.name.split("/"):
                    raise SystemExit("unsafe archive member: " + m.name)
            t.extractall(os.path.join(dest, "x"))
        top = glob.glob(os.path.join(dest, "x", "*", "src", "stylesheet"))
        if not top or not os.path.exists(os.path.join(top[0], "gtk.scss")):
            raise SystemExit("archive has no src/stylesheet/gtk.scss (upstream layout changed?)")
        os.rename(top[0], os.path.join(dest, "stylesheet"))
        shutil.rmtree(os.path.join(dest, "x"), ignore_errors=True)
        os.remove(tgz)
    info = {"ref": ref, "is_tag": is_tag, "commit": None}
    meson = http_get("%s/GNOME/libadwaita/-/raw/%s/meson.build" % (GITLAB, urllib.parse.quote(ref)))
    m = re.search(r"version:\s*'([^']+)'", meson)
    info["version"] = m.group(1) if m else ref
    if not is_tag:
        try:
            info["commit"] = json.loads(http_get("%s/api/v4/projects/%s/repository/commits/%s" % (
                GITLAB, PROJECT, urllib.parse.quote(ref, safe=""))))["short_id"]
        except Exception:
            pass
    return os.path.join(dest, "stylesheet"), info


# --------------------------------------------------------------------------------------------------- sass
def find_sass(cache):
    """Return compile(source, include_dir) -> css. libsass module, else sassc / sass binary, else pip libsass."""
    def module():
        import sass  # type: ignore
        return lambda src, inc: sass.compile(string=src, include_paths=[inc])
    for sp in glob.glob(os.path.join(cache, "venv", "lib", "python*", "site-packages")):
        sys.path.insert(0, sp)
    try:
        return module()
    except ImportError:
        pass
    for exe, args in (("sassc", ["-s", "-I"]), ("sass", ["--stdin", "-I"])):
        path = shutil.which(exe)
        if path:
            def run(src, inc, path=path, args=args):
                p = subprocess.run([path] + args + [inc], input=src, capture_output=True, text=True)
                if p.returncode:
                    raise SystemExit("%s failed:\n%s" % (exe, p.stderr))
                return p.stdout
            return run
    print("no sass compiler found: installing libsass (pip --user, else a venv in the cache)")
    pip = subprocess.run([sys.executable, "-m", "pip", "install", "--user", "--quiet", "libsass"],
                         capture_output=True, text=True)
    if pip.returncode:
        venv = os.path.join(cache, "venv")
        if not os.path.isdir(venv):
            if shutil.which("uv"):
                subprocess.run(["uv", "venv", venv], check=True, capture_output=True)
                subprocess.run(["uv", "pip", "install", "--python", venv + "/bin/python", "libsass"],
                               check=True, capture_output=True)
            else:
                subprocess.run([sys.executable, "-m", "venv", venv], check=True)
                subprocess.run([venv + "/bin/python", "-m", "pip", "install", "--quiet", "libsass"], check=True)
        for sp in glob.glob(venv + "/lib/python*/site-packages"):
            sys.path.insert(0, sp)
    import importlib
    importlib.invalidate_caches()
    try:
        return module()
    except ImportError:
        raise SystemExit("cannot get a sass compiler: install sassc (apt install sassc) or `pip install libsass`")


# --------------------------------------------------------------------------------------------------- CSS reading
class Css:
    """The compiled stylesheet as flat lists. media is None or e.g. 'prefers-color-scheme: dark'."""

    def __init__(self, text):
        self.rules = []        # (media, [selectors], {prop: value})
        self.defines = []      # (media, name, value) from @define-color
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        self._parse(text, 0, None, top=True)

    @staticmethod
    def _decls(body):
        out, depth, cur = {}, 0, ""
        for ch in body + ";":
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            if ch == ";" and depth == 0:
                if ":" in cur:
                    k, v = cur.split(":", 1)
                    out[k.strip()] = " ".join(v.split())
                cur = ""
            else:
                cur += ch
        return out

    def _parse(self, t, i, media, top=False):
        n = len(t)
        while i < n:
            j = i
            depth = 0
            while j < n and not (t[j] in "{;}" and depth == 0):
                depth += (t[j] == "(") - (t[j] == ")")
                j += 1
            pre = " ".join(t[i:j].split())
            if j >= n:
                break
            if t[j] == "}":
                return j + 1
            if t[j] == ";":
                m = re.match(r"@define-color\s+(\S+)\s+(.*)$", pre)
                if m:
                    self.defines.append((media, m.group(1), m.group(2)))
                i = j + 1
                continue
            # '{'
            if pre.startswith("@media"):
                i = self._parse(t, j + 1, pre[6:].strip().strip("()"), False)
            elif pre.startswith("@"):
                # other block at-rule (keyframes, ...): skip balanced
                d, k = 1, j + 1
                while k < n and d:
                    d += (t[k] == "{") - (t[k] == "}")
                    k += 1
                i = k
            else:
                k = t.index("}", j)
                sels = [" ".join(s.split()) for s in re.split(r",(?![^()]*\))", pre)]
                self.rules.append((media, sels, self._decls(t[j + 1:k])))
                i = k + 1
        return n

    def find(self, selector, prop, media=None):
        """Value of `prop` in the last rule whose selector list contains `selector` exactly (same media)."""
        val = None
        for m, sels, decls in self.rules:
            if m == media and selector in sels and prop in decls:
                val = decls[prop]
        return val

    def find_re(self, selector_re, prop, media=None):
        val = None
        for m, sels, decls in self.rules:
            if m == media and prop in decls and any(re.match(selector_re, s) for s in sels):
                val = decls[prop]
        return val

    def root_vars(self, media=None):
        out = {}
        for m, sels, decls in self.rules:
            if m == media and ":root" in sels:
                out.update({k: v for k, v in decls.items() if k.startswith("--")})
        return out

    def named_colors(self, dark):
        out = {}
        for m, name, val in self.defines:
            if m is None or (dark and m == "prefers-color-scheme: dark"):
                out[name] = val
        return out


# --------------------------------------------------------------------------------------------------- values
def parse_color(s, names=None, depth=0):
    """CSS colour text -> {"r","g","b","a"} (r,g,b ints 0-255, a float)."""
    if s is None:
        raise ValueError("selector or property not found upstream")
    s = s.strip()
    if s.startswith("@"):
        if names is None or s[1:] not in names or depth > 8:
            raise ValueError("unresolved reference " + s)
        return parse_color(names[s[1:]], names, depth + 1)
    m = re.fullmatch(r"#([0-9a-fA-F]{3,8})", s)
    if m:
        h = m.group(1)
        if len(h) in (3, 4):
            h = "".join(c * 2 for c in h)
        v = [int(h[i:i + 2], 16) for i in range(0, len(h), 2)]
        return {"r": v[0], "g": v[1], "b": v[2], "a": float(v[3]) / 255 if len(v) == 4 else 1.0}
    if s.lower() == "white":
        return {"r": 255, "g": 255, "b": 255, "a": 1.0}
    if s.lower() == "black":
        return {"r": 0, "g": 0, "b": 0, "a": 1.0}
    m = re.fullmatch(r"(?i)rgba?\(\s*([\d.]+)[\s,]+([\d.]+)[\s,]+([\d.]+)\s*(?:[/,]\s*([\d.]+)(%?))?\s*\)", s)
    if m:
        a = 1.0
        if m.group(4) is not None:
            a = float(m.group(4)) / 100 if m.group(5) else float(m.group(4))
        return {"r": int(float(m.group(1))), "g": int(float(m.group(2))), "b": int(float(m.group(3))), "a": a}
    raise ValueError("cannot parse colour: " + s)


def px(s):
    if s is None:
        raise ValueError("selector or property not found upstream")
    if s.strip() == "0":
        return 0
    m = re.fullmatch(r"(-?[\d.]+)px", s.strip())
    if not m:
        raise ValueError("not a px value: " + s)
    f = float(m.group(1))
    return int(f) if f == int(f) else f


def pct(s):
    """'15%' -> 0.15 ; 'color-mix(in srgb, currentColor 7%, transparent)' -> 0.07 ; 'currentColor' -> 1.0"""
    if s is None:
        raise ValueError("selector or property not found upstream")
    if s.strip() == "currentColor":
        return 1.0
    m = re.search(r"(?:currentColor|var\([^)]*\))\s+([\d.]+)%", s) or re.fullmatch(r"\s*([\d.]+)%\s*", s)
    if not m:
        raise ValueError("no percentage in: " + s)
    return float(m.group(1)) / 100


def px_list(s):
    """'5px 10px' -> [5,10,5,10] (CSS shorthand order)"""
    if s is None:
        raise ValueError("selector or property not found upstream")
    v = [px(x) for x in s.split()]
    return {1: v * 4, 2: v * 2, 3: v + [v[1]], 4: v}[len(v)]


def hexs(c):
    return "#%02x%02x%02x" % (c["r"], c["g"], c["b"])


def srgb_to_lin(v):
    return v / 12.92 if v < 0.04045 else ((v + 0.055) / 1.055) ** 2.4


def lin_to_srgb(v):
    return 12.92 * v if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055


def standalone(c, dark, M):
    """libadwaita's oklab(from <bg> min(l,0.5) a b) / max(l,0.85) (adw-color-utils.c); M = JSON oklab_matrices."""
    lin = [srgb_to_lin(c[k] / 255) for k in "rgb"]
    lms = [sum(M["lin_to_lms"][i][j] * lin[j] for j in range(3)) for i in range(3)]
    lab = [sum(M["lms_cbrt_to_lab"][i][j] * math.copysign(abs(lms[j]) ** (1 / 3), lms[j]) for j in range(3)) for i in range(3)]
    lab[0] = max(lab[0], 0.85) if dark else min(lab[0], 0.5)
    lms2 = [sum(M["lab_to_lms_cbrt"][i][j] * lab[j] for j in range(3)) ** 3 for i in range(3)]
    lin2 = [sum(M["lms_to_lin"][i][j] * lms2[j] for j in range(3)) for i in range(3)]
    out = [min(1.0, max(0.0, lin_to_srgb(v))) for v in lin2]
    return "#%02x%02x%02x" % tuple(round(v * 255) for v in out)


def mix(a, wa, b):
    """color-mix(in srgb, a wa%, b) ignoring alpha, as the JSON's derived_hex does"""
    c = {k: round(a[k] * wa + b[k] * (1 - wa)) for k in "rgb"}
    return hexs(c)


# --------------------------------------------------------------------------------------------------- extraction
# Sass variables the extraction needs (evaluated by the same compile as the stylesheet).
SASS_VARS = [
    "button_radius", "card_radius", "menu_radius", "popover_radius", "dialog_radius", "alert_radius",
    "border_opacity", "border_opacity_hc", "focus_border_opacity", "focus_border_opacity_hc",
    "dimmer_opacity", "dimmer_opacity_hc", "strong_disabled_opacity",
    "hover_color", "active_color", "selected_color", "selected_hover_color", "selected_active_color",
    "view_hover_color", "view_active_color",
    "view_selected_color", "view_selected_hover_color", "view_selected_active_color",
    "trough_color", "trough_hover_color", "trough_active_color",
    "trough_color_hc", "trough_hover_color_hc", "trough_active_color_hc",
    "slider_hover_color", "osd_fg_color", "osd_bg_color", "toast_bg_color", "toast_fg_color",
    "tooltip_border_color", "window_outline_color", "window_outline_color_hc",
]


class Extractor:
    def __init__(self, css, svars, old):
        self.css, self.sv, self.old = css, svars, old
        self.derived = set()    # JSON paths filled from upstream
        self.warnings = []

    def put(self, new, path, fn, what):
        """Set new[path] = fn(); on failure keep the old value and warn (what = where upstream was expected)."""
        keys = path.split("/")
        try:
            val = fn()
            if val is None:
                raise ValueError("not found")
        except Exception as e:
            self.warnings.append("%s: kept old value (%s: %s)" % (path, what, e))
            return
        d = new
        for k in keys[:-1]:
            d = d.setdefault(k, {})
        d[keys[-1]] = val
        self.derived.add(path)

    def sassv(self, name):
        v = self.sv.get(name)
        if v is None:
            raise ValueError("Sass variable $%s not defined" % name)
        return v


def build(css, svars, old):
    new = copy.deepcopy(old)
    X = Extractor(css, svars, old)
    put, F = X.put, css.find
    M = old["accent"]["standalone_algorithm"]["oklab_matrices"]

    # --- accent palette (:root --accent-<name>)
    root = css.root_vars()
    for name in old["accent"]["palette"]:
        put(new, "accent/palette/" + name, lambda n=name: parse_color(root["--accent-" + n]), ":root --accent-" + name)
    palette = new["accent"]["palette"]
    new_accents = [k[len("--accent-"):] for k in root if re.fullmatch(r"--accent-(?!bg|fg|color)[a-z]+", k)]
    for n in new_accents:
        if n not in palette:
            palette[n] = parse_color(root["--accent-" + n])
            X.derived.add("accent/palette/" + n)
            X.warnings.append("NEW accent colour upstream: %s (add it to the code that lists accents if needed)" % n)
    for n in list(palette):
        if n not in new_accents:
            X.warnings.append("accent colour %s no longer in the stylesheet (kept)" % n)
    for mode in ("light", "dark"):
        for n in palette:
            put(new, "accent/standalone_resolved_hex/%s/%s" % (mode, n),
                lambda n=n, mode=mode: standalone(palette[n], mode == "dark", M), "computed")

    # --- base palette ($blue_1 ...)
    nc = css.named_colors(False)
    for fam in old["base_palette"]:
        if fam == "src":
            continue
        put(new, "base_palette/" + fam, lambda f=fam: [hexs(parse_color(nc["%s_%d" % (f, i)])) for i in range(1, 6)],
            "@define-color %s_1..5" % fam)

    # --- per scheme colours, opacities, derived values
    root_dark = css.root_vars("prefers-color-scheme: dark")
    root_hc = css.root_vars("prefers-contrast: more")
    template_keys = list(old["schemes"]["light"]["colors"])
    for sch in SCHEMES:
        dark = sch.endswith("dark")
        hc = sch.startswith("hc-")
        names = css.named_colors(dark)
        vars_ = dict(root)
        if dark:
            vars_.update(root_dark)
        colors = {}
        for k in template_keys:
            def get(k=k):
                if k + "_color" in names:
                    return parse_color(names[k + "_color"], names)
                v = vars_.get("--" + k.replace("_", "-") + "-color")
                if v is None:
                    raise ValueError("no @define-color %s_color / --%s-color" % (k, k.replace("_", "-")))
                return parse_color(v)
            put(new, "schemes/%s/colors/%s" % (sch, k), get, "@define-color %s_color" % k)
        # colours upstream that the JSON does not have yet
        for nm in names:
            m = re.fullmatch(r"(.+_(?:bg|fg|shade|backdrop|border|outline))_color", nm)
            if m and not nm.startswith(("theme_", "insensitive_", "unfocused_")) and m.group(1) not in template_keys and not re.fullmatch(
                    r"(accent|destructive|success|warning|error)", m.group(1)) and not re.fullmatch(r"[a-z]+_\d", nm):
                key = m.group(1)
                try:
                    new["schemes"][sch]["colors"][key] = parse_color(names[nm], names)
                    X.derived.add("schemes/%s/colors/%s" % (sch, key))
                    X.warnings.append("NEW colour upstream: %s (%s) added to the JSON; code may want to use it" % (key, sch))
                except ValueError:
                    pass
        # opacities
        put(new, "schemes/%s/opacities/border" % sch,
            lambda: pct((root_hc if hc else root)["--border-opacity"]), ":root --border-opacity")
        put(new, "schemes/%s/opacities/dim" % sch, lambda: pct((root_hc if hc else root)["--dim-opacity"]), ":root --dim-opacity")
        put(new, "schemes/%s/opacities/disabled" % sch,
            lambda: pct((root_hc if hc else root)["--disabled-opacity"]), ":root --disabled-opacity")
        put(new, "schemes/%s/opacities/focus_ring_alpha" % sch,
            lambda: pct(X.sassv("focus_border_opacity_hc" if hc else "focus_border_opacity")), "$focus_border_opacity")
        put(new, "schemes/%s/opacities/dimmer" % sch,
            lambda: pct(X.sassv("dimmer_opacity_hc" if hc else "dimmer_opacity")), "$dimmer_opacity")
        put(new, "schemes/%s/opacities/strong_disabled" % sch, lambda: pct(X.sassv("strong_disabled_opacity")), "$strong_disabled_opacity")
        # standalone colours
        c = new["schemes"][sch]["colors"]
        for nm in ("destructive", "success", "warning", "error"):
            put(new, "schemes/%s/standalone_semantic_colors_hex/%s_color" % (sch, nm),
                lambda nm=nm: standalone(c[nm + "_bg"], dark, M), "computed from %s_bg" % nm)
        for n in palette:
            put(new, "schemes/%s/accent_standalone_hex/%s" % (sch, n), lambda n=n: standalone(palette[n], dark, M), "computed")
        put(new, "schemes/%s/derived_hex/opaque_button_default_bg" % sch,
            lambda: mix(c["window_bg"], 0.85, c["window_fg"]),
            "computed: mix(window-bg 85%, window-fg 15%) (_buttons.scss)")
        put(new, "schemes/" + sch + "/derived_hex/insensitive_bg_color(window 60% + view)",
            lambda: mix(c["window_bg"], 0.6, c["view_bg"]), "computed (_compat-colors.scss)")

    # --- other colours
    S = X.sassv
    put(new, "other_colors/border_color/light", lambda: pct(S("border_opacity")), "$border_opacity")
    put(new, "other_colors/border_color/hc", lambda: pct(S("border_opacity_hc")), "$border_opacity_hc")
    for key, var in (("hover", "hover_color"), ("active", "active_color"), ("selected", "selected_color"),
                     ("selected_hover", "selected_hover_color"), ("selected_active", "selected_active_color"),
                     ("view_hover", "view_hover_color"), ("view_active", "view_active_color")):
        put(new, "other_colors/" + key, lambda var=var: pct(S(var)), "$" + var)
    put(new, "other_colors/view_selected_accent",
        lambda: [pct(S(v)) for v in ("view_selected_color", "view_selected_hover_color", "view_selected_active_color")],
        "$view_selected*_color")
    for key, var in (("normal", "trough_color"), ("hover", "trough_hover_color"), ("active", "trough_active_color")):
        put(new, "other_colors/trough/" + key, lambda var=var: pct(S(var)), "$" + var)
    put(new, "other_colors/trough/hc", lambda: [pct(S(v)) for v in ("trough_color_hc", "trough_hover_color_hc", "trough_active_color_hc")],
        "$trough_*_hc")
    put(new, "other_colors/slider_hover", lambda: hexs(parse_color(S("slider_hover_color"))), "$slider_hover_color")
    for key, var in (("osd_fg", "osd_fg_color"), ("osd_bg", "osd_bg_color"), ("toast_bg", "toast_bg_color"),
                     ("toast_fg", "toast_fg_color"), ("tooltip_border", "tooltip_border_color")):
        put(new, "other_colors/%s" % key, lambda var=var: parse_color(S(var)), "$" + var)
    put(new, "other_colors/window_outline/normal", lambda: parse_color(S("window_outline_color")), "$window_outline_color")
    put(new, "other_colors/window_outline/hc", lambda: parse_color(S("window_outline_color_hc")), "$window_outline_color_hc")
    put(new, "other_colors/osd_button_bg", lambda: parse_color(F("button.osd", "background-color") or
                                                              F("menubutton.osd > button, button.osd", "background-color")),
        "button.osd background-color")
    put(new, "other_colors/tooltip_bg", lambda: parse_color(F("tooltip.background", "background-color")), "tooltip.background")
    put(new, "other_colors/tooltip_fg", lambda: parse_color(F("tooltip.background", "color")), "tooltip.background")
    put(new, "other_colors/banner_color", lambda: parse_color(F("banner", "--banner-color")), "banner --banner-color")

    # --- radii
    radii = {
        "button": lambda: px(S("button_radius")),
        "card": lambda: px(S("card_radius")),
        "menu_item": lambda: px(S("menu_radius")),
        "popover": lambda: px(S("popover_radius")),
        "dialog_sheet": lambda: px(S("dialog_radius")),
        "window_csd": lambda: px(root["--window-radius"]),
        "alert_dialog": lambda: px(S("alert_radius")),
        "switch": lambda: px(F("switch", "border-radius")),
        "check": lambda: px(F("check", "border-radius")),
        "checkbutton_focus": lambda: px(F("checkbutton", "border-radius")),
        "circular_pill": lambda: px(F("button.circular", "border-radius")) if F("button.circular", "border-radius")
        else px(css.find_re(r"^button\.circular\b", "border-radius")),
        "tab_thumbnail": lambda: px(F("tabthumbnail", "border-radius")),
        "toast": lambda: px(F("toast", "border-radius")),
        "frame": lambda: px(F("frame", "border-radius")),
    }
    for k, fn in radii.items():
        put(new, "radii/" + k, fn, "radius")

    # --- widgets used by the style code
    sb = "scrollbar > range > trough > slider"
    put(new, "widgets/scrollbar/slider_min_length", lambda: px(F("scrollbar.horizontal > range > trough > slider", "min-width")), sb)
    put(new, "widgets/scrollbar/slider_width_expanded", lambda: px(F(sb, "min-width")), sb)
    put(new, "widgets/scrollbar/slider_width_overlay_idle",
        lambda: px(F("scrollbar.overlay-indicator:not(.hovering) > range > trough > slider", "min-width")), "overlay idle slider")
    put(new, "widgets/scrollbar/trough_margin/base", lambda: px(F("scrollbar > range > trough", "margin")), "scrollbar trough")
    put(new, "widgets/scrollbar/trough_margin/along_axis", lambda: px(F("scrollbar > range > trough", "margin")), "scrollbar trough")
    put(new, "widgets/scrollbar/trough_margin/expanded_cross_axis",
        lambda: px(F("scrollbar.horizontal > range > trough", "margin-top")), "scrollbar.horizontal trough")
    put(new, "widgets/scrollbar/trough_margin/overlay_idle_cross_axis",
        lambda: px(F("scrollbar.overlay-indicator.horizontal:not(.hovering) > range > trough", "margin-top")), "overlay idle trough")
    sbw = new["widgets"]["scrollbar"]
    put(new, "widgets/scrollbar/cross_axis_total_px/expanded",
        lambda: 2 * sbw["trough_margin"]["expanded_cross_axis"] + sbw["slider_width_expanded"], "computed")
    put(new, "widgets/scrollbar/cross_axis_total_px/overlay_idle",
        lambda: 2 * sbw["trough_margin"]["overlay_idle_cross_axis"] + sbw["slider_width_overlay_idle"], "computed")
    put(new, "widgets/scrollbar/slider_radius", lambda: px(F(sb, "border-radius")), sb)
    put(new, "widgets/scrollbar/slider_bg/normal", lambda: "currentColor %.2f" % pct(F(sb, "background-color")), sb)
    put(new, "widgets/scrollbar/slider_bg/hover", lambda: pct(F(sb + ":hover", "background-color")), sb + ":hover")
    put(new, "widgets/scrollbar/slider_bg/active", lambda: pct(F(sb + ":active", "background-color")), sb + ":active")
    hcm = "prefers-contrast: more"
    put(new, "widgets/scrollbar/slider_bg/hc",
        lambda: [pct(F(sb, "background-color", hcm)), pct(F(sb + ":hover", "background-color", hcm)),
                 pct(F(sb + ":active", "background-color", hcm))], sb + " (prefers-contrast)")

    put(new, "widgets/toggle_group/radius", lambda: px(F("toggle-group", "border-radius")), "toggle-group")
    put(new, "widgets/scale/trough_thickness", lambda: px(F("scale", "min-height")), "scale")
    put(new, "widgets/scale/padding", lambda: px(F("scale", "padding")), "scale")
    put(new, "widgets/scale/slider", lambda: px(F("scale > trough > slider", "min-width")), "scale > trough > slider")
    put(new, "widgets/scale/slider_margin", lambda: px(F("scale > trough > slider", "margin")), "scale > trough > slider")
    put(new, "widgets/scale/radius", lambda: px(css.find_re(r"^scale > trough$", "border-radius")), "scale > trough")
    put(new, "widgets/scale/marks/length", lambda: px(F("scale.horizontal > marks indicator", "min-height")), "scale marks")
    put(new, "widgets/scale/marks/distance", lambda: px(F("scale.horizontal > marks.top", "margin-bottom")), "scale marks")
    put(new, "widgets/scale/marks/width", lambda: px(F("scale.horizontal > marks indicator", "min-width")), "scale marks")
    put(new, "widgets/progressbar/thickness",
        lambda: px(css.find_re(r"^progressbar\.horizontal > trough$", "min-height")), "progressbar.horizontal")
    put(new, "widgets/progressbar/min_length_h", lambda: px(css.find_re(r"^progressbar\.horizontal > trough$", "min-width")), "progressbar")
    put(new, "widgets/progressbar/min_length_v", lambda: px(css.find_re(r"^progressbar\.vertical > trough$", "min-height")), "progressbar")
    put(new, "widgets/progressbar/radius", lambda: px(css.find_re(r"^progressbar > trough$", "border-radius")), "progressbar > trough")
    put(new, "widgets/progressbar/osd_thickness",
        lambda: px(css.find_re(r"^progressbar\.osd\.horizontal > trough$", "min-height")), "progressbar.osd")
    put(new, "widgets/spinbutton/padding", lambda: px(F("spinbutton", "padding")), "spinbutton")
    put(new, "widgets/spinbutton/text_min_width", lambda: px(F("spinbutton:not(.vertical) > text", "min-width")), "spinbutton text")
    put(new, "widgets/spinbutton/text_padding", lambda: px(F("spinbutton:not(.vertical) > text", "padding")), "spinbutton text")
    sbtn = r"^spinbutton:not\(\.vertical\) > button\.image-button\.up\b"
    put(new, "widgets/spinbutton/button_min",
        lambda: [px(css.find_re(sbtn, "min-width")), px(css.find_re(sbtn, "min-height"))], "spinbutton button")
    put(new, "widgets/spinbutton/vertical/text_min",
        lambda: [px(F("spinbutton.vertical > text", "min-width")), px(F("spinbutton.vertical > text", "min-height"))],
        "spinbutton.vertical text")
    put(new, "widgets/button/min_height", lambda: px(F("button", "min-height")), "button")
    put(new, "widgets/button/min_width", lambda: px(F("button", "min-width")), "button")
    put(new, "widgets/button/padding", lambda: px_list(F("button", "padding")), "button")
    put(new, "widgets/button/radius", lambda: px(S("button_radius")), "$button_radius")
    return new, X


# --------------------------------------------------------------------------------------------------- diff / report
def is_color(v):
    return isinstance(v, dict) and set(v) == {"r", "g", "b", "a"}


def leaves(x, p=""):
    if isinstance(x, dict) and not is_color(x):
        for k, v in x.items():
            yield from leaves(v, p + "/" + k if p else k)
    else:
        yield p, x


def show(v):
    if is_color(v):
        return hexs(v) if v["a"] == 1 else "rgba(%d,%d,%d,%g)" % (v["r"], v["g"], v["b"], v["a"])
    s = json.dumps(v)
    return s if len(s) < 70 else s[:67] + "..."


def diff(old, new):
    o, n = dict(leaves(old)), dict(leaves(new))
    ch = [(p, o[p], n[p]) for p in n if p in o and o[p] != n[p]]
    add = [(p, n[p]) for p in n if p not in o]
    rem = [(p, o[p]) for p in o if p not in n]
    return ch, add, rem


def manual_paths(new, derived):
    return [p for p, _ in leaves(new) if not p.startswith("_meta") and p not in derived and
            not any(p == d or p.startswith(d + "/") for d in derived)]


def make_report(old, new, X, info, fname, full_manual):
    ch, add, rem = diff(old, new)
    L = []
    L.append("## libadwaita %s -> %s" % (old["_meta"]["libadwaita"], new["_meta"]["libadwaita"]))
    L.append("")
    L.append("Token file: `data/%s`. Values read from upstream: %d, carried over unchanged: %d." % (
        fname, len(X.derived), len(manual_paths(new, X.derived))))
    L.append("")
    L.append("### Changed values (%d)" % len([c for c in ch if not c[0].startswith("_meta")]))
    L += ["- `%s`: %s -> %s" % (p, show(a), show(b)) for p, a, b in ch if not p.startswith("_meta")] or ["- none"]
    if add:
        L += ["", "### New keys (%d)" % len(add)] + ["- `%s`: %s" % (p, show(v)) for p, v in add]
    if rem:
        L += ["", "### Removed keys (%d)" % len(rem)] + ["- `%s`: %s" % (p, show(v)) for p, v in rem]
    if X.warnings:
        L += ["", "### Warnings (upstream changed shape, old values kept)"] + ["- " + w for w in X.warnings]
    man = manual_paths(new, X.derived)
    sections = {}
    for p in man:
        sections.setdefault("/".join(p.split("/")[:2]) if p.split("/")[0] in ("widgets", "typography") else p.split("/")[0], []).append(p)
    L += ["", "### Manual (not derivable from the stylesheet; carried over, a human should check)"]
    L.append("Hand-measured or described in prose, line references (`src` keys), and values defined in libadwaita's C code "
             "(accent_fg, default accent, portal mapping). New upstream rules for these widgets are NOT picked up here.")
    L += ["- `%s`: %d values" % (k, len(v)) for k, v in sorted(sections.items())]
    if full_manual:
        L += [""] + ["  - " + p for p in man]
    if old["_meta"].get("gnome") == new["_meta"].get("gnome") and old["_meta"]["libadwaita"] != new["_meta"]["libadwaita"]:
        L += ["- `_meta/gnome`: still \"%s\" (set it by hand, or pass --gnome)" % new["_meta"]["gnome"]]
    return "\n".join(L)


# --------------------------------------------------------------------------------------------------- main
def current_meta():
    with open(tokensource.active_path()) as f:
        return json.load(f)


def run(cmd, **kw):
    print("$ " + " ".join(cmd))
    return subprocess.run(cmd, **kw)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0], formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ref", nargs="?", help="libadwaita tag or branch (1.10.0, main), or 'latest' for the newest stable tag")
    ap.add_argument("--check", action="store_true", help="only compare the active token file with the newest stable tag; "
                    "prints current=, latest=, newer= lines")
    ap.add_argument("--dry-run", action="store_true", help="write nothing into the repo, only report")
    ap.add_argument("--apply", action="store_true",
                    help="after writing the JSON: set data/current, regenerate header and KDE schemes, build check")
    ap.add_argument("--no-build", action="store_true", help="with --apply: skip the build check")
    ap.add_argument("--base", help="previous token JSON used as template (default: the active one)")
    ap.add_argument("--gnome", help="GNOME release number for _meta.gnome (not derivable from the stylesheet)")
    ap.add_argument("--report", help="also write the report (markdown) to this file")
    ap.add_argument("--list-manual", action="store_true", help="list every carried-over key, not only the counts")
    ap.add_argument("--cache", default=os.environ.get("LIBADWAITA_CACHE") or os.path.join(tempfile.gettempdir(), "qt-libadwaita-update"),
                    help="download/cache directory (default: $LIBADWAITA_CACHE or <tmp>/qt-libadwaita-update)")
    ap.add_argument("--build-dir", help="build directory for the build check (default: <cache>/build)")
    a = ap.parse_args()
    os.makedirs(a.cache, exist_ok=True)

    cur = current_meta()
    if a.check:
        latest = latest_stable_tag()
        curv = cur["_meta"]["libadwaita"]
        print("current=%s\nlatest=%s\nnewer=%s" % (curv, latest, "true" if vtuple(latest) > vtuple(curv) else "false"))
        return 0
    if not a.ref:
        ap.error("give a tag or branch (e.g. 1.10.0, main, latest)")
    ref = latest_stable_tag() if a.ref == "latest" else a.ref
    print("ref: " + ref)
    old = json.load(open(a.base)) if a.base else cur

    srcdir, info = fetch_sources(ref, a.cache)
    sass = find_sass(a.cache)
    code = "@import 'gtk';\n" + "".join(".v-%s { v: #{$%s}; }\n" % (n, n) for n in SASS_VARS)
    try:
        text = sass(code, srcdir)
    except Exception as e:
        raise SystemExit("compiling the stylesheet failed (a Sass variable renamed upstream, or a Sass feature this "
                         "compiler lacks; try sassc/dart-sass):\n%s" % e)
    css = Css(text)
    svars = {}
    for m, sels, decls in css.rules:
        for s in sels:
            if s.startswith(".v-") and "v" in decls:
                svars[s[3:]] = decls["v"]
    print("stylesheet compiled: %d rules, %d named colours" % (len(css.rules), len(css.defines)))

    new, X = build(css, svars, old)
    ver = info["version"]
    new["_meta"]["libadwaita"] = ver
    tag_txt = "libadwaita %s" % ver + (" (%s@%s)" % (ref, info["commit"]) if not info["is_tag"] and info["commit"] else "")
    new["_meta"]["source_tag"] = re.sub(r"^libadwaita \S+( \([^)]*@[^)]*\))?", tag_txt, old["_meta"]["source_tag"], count=1)
    if a.gnome:
        new["_meta"]["gnome"] = a.gnome
    mm = re.match(r"(\d+)\.(\d+)", ver)
    fname = "adwaita-%s.%s.json" % (mm.group(1), mm.group(2)) if mm else "adwaita-%s.json" % ref
    text_out = json.dumps(new, indent=1)

    rep = make_report(old, new, X, info, fname, a.list_manual)
    print("\n" + rep)
    if a.report:
        open(a.report, "w").write(rep + "\n")

    if a.dry_run:
        tmp = os.path.join(a.cache, fname)
        open(tmp, "w").write(text_out)
        print("\ndry run: JSON written to %s only" % tmp)
        return 0
    out = os.path.join(DATA, fname)
    open(out, "w").write(text_out)
    print("\nwrote " + out)
    if not a.apply:
        print("next: tools/update_libadwaita.py %s --apply   (or edit data/current and run the generators yourself)" % a.ref)
        return 0

    # ---- apply
    open(os.path.join(DATA, "current"), "w").write(fname + "\n")
    print("data/current -> " + fname)
    for gen in ("gen_tokens.py", "gen_kde_colors.py"):
        if run([sys.executable, os.path.join(HERE, gen)]).returncode:
            raise SystemExit(gen + " failed")
    if not a.no_build:
        bd = a.build_dir or os.path.join(a.cache, "build")
        if run(["cmake", "-S", ROOT, "-B", bd, "-DCMAKE_BUILD_TYPE=Release"]).returncode or \
                run(["cmake", "--build", bd, "-j4"]).returncode:
            raise SystemExit("build check FAILED (a new token may need code changes)")
        print("build check: OK")
    if os.path.isdir(os.path.join(ROOT, ".git")) or os.path.isfile(os.path.join(ROOT, ".git")):
        print("\nchanged files:")
        r = run(["git", "-C", ROOT, "status", "--short", "--", "data", "src"], capture_output=True, text=True)
        print(r.stdout.rstrip() or "  (none: regenerated files are identical to the committed ones)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
