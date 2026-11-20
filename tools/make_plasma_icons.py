#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Build the "Libadwaita" and "Libadwaita-Dark" icon themes for KDE from the system's Adwaita icon theme.

No icon is drawn here. The script only
  * copies Adwaita's symbolic SVGs and makes them follow the KDE colour scheme (the way Breeze icons do it:
    a <style id="current-color-scheme"> block plus class="ColorScheme-Text" and fill="currentColor"), because the
    originals have a hard-coded dark grey that would be invisible in a dark colour scheme;
  * adds the plain names KDE asks for (document-open -> document-open-symbolic) where Adwaita only has the
    symbolic icon; Adwaita's own full-colour icons (folders, devices, mime types, apps) are NOT copied: the
    new theme inherits them from Adwaita;
  * writes index.theme with Inherits=Adwaita,breeze(-dark),hicolor so everything Adwaita lacks comes from Breeze.

  make_plasma_icons.py [--out DIR] [--src ADWAITA_DIR]       (default out: ~/.local/share/icons)
  make_plasma_icons.py --remove [--out DIR]

Files are replaced atomically (write a temp name, rename): running programs may have icons open.
Exit status 3 means the Adwaita icon theme is not installed (nothing is written).
"""
import argparse, os, re, shutil, sys

THEMES = [
    # name, default text colour when no KDE recolouring happens, breeze variant to inherit from
    ("Libadwaita", "#2e3436", "breeze"),
    ("Libadwaita-Dark", "#eeeeec", "breeze-dark"),
]
# colours Adwaita uses for the monochrome glyph (everything else, e.g. green/orange/red status colours, stays)
GLYPH = {"#2e3436", "#2e3434", "#222222", "#474747", "#000", "#000000", "#bebebe", "#666666", "#505050"}
SYMBOLIC_DIRS = ["actions", "apps", "categories", "devices", "emblems", "emotes", "legacy", "mimetypes", "places", "status", "ui"]
FULLCOLOUR_DIRS = ["16x16/devices", "16x16/emblems", "16x16/mimetypes", "16x16/places",
                   "scalable/devices", "scalable/mimetypes", "scalable/places", "scalable/status"]
# KDE (Breeze) names without an exact Adwaita twin -> Adwaita symbolic name (used only if the target exists)
EXTRA_ALIASES = {
    "view-list-icons": "view-grid", "view-list-details": "view-list", "view-list-text": "view-list",
    "dialog-ok": "emblem-ok", "dialog-ok-apply": "emblem-ok", "dialog-cancel": "window-close", "dialog-close": "window-close",
    "configure": "emblem-system", "preferences-system": "emblem-system", "system-search": "edit-find",
    "application-menu": "open-menu", "overflow-menu": "view-more", "view-more-horizontal": "view-more",
    "go-up-search": "go-up", "edit-find-replace": "edit-find",
    "help-contents": "help-browser", "tools-report-bug": "bug",
    "view-visible": "eye-open-negative-filled",
    "view-hidden": "eye-not-looking", }
PKG_HINT = ("adwaita-icon-theme (Debian, Ubuntu, Mint, Arch, Fedora, openSUSE: package name 'adwaita-icon-theme';"
            " Alpine: 'adwaita-icon-theme')")


def find_adwaita(explicit):
    cands = [explicit] if explicit else []
    dirs = [os.path.expanduser("~/.local/share")] + os.environ.get("XDG_DATA_DIRS", "/usr/local/share:/usr/share").split(":")
    for d in dirs + ["/usr/share"]:
        cands.append(os.path.join(d, "icons", "Adwaita"))
    for c in cands:
        if c and os.path.isfile(os.path.join(c, "index.theme")) and os.path.isdir(os.path.join(c, "symbolic")):
            return c
    return None


def recolour(svg, default):
    """Make the monochrome glyph follow the colour scheme. Returns None only if the file has no <svg> element."""
    if "current-color-scheme" in svg:
        return svg
    changed = False

    def fix_tag(m):
        nonlocal changed
        tag = m.group(0)
        name = m.group(1)
        if name in ("svg", "linearGradient", "radialGradient", "stop", "style", "defs", "clipPath", "mask", "filter"):
            return tag
        new = tag
        hit = False

        def attr_fill(mm):
            nonlocal hit
            if mm.group(2).lower() in GLYPH:
                hit = True
                return mm.group(1) + 'fill="currentColor"'
            return mm.group(0)
        new = re.sub(r'(\s)fill="(#[0-9a-fA-F]{3,6})"', attr_fill, new)

        def style_fill(mm):
            nonlocal hit
            if mm.group(2).lower() in GLYPH:
                hit = True
                return mm.group(1) + "fill:currentColor"
            return mm.group(0)
        new = re.sub(r"(^|[\s\";])fill:(#[0-9a-fA-F]{3,6})", style_fill, new)
        if not hit:
            return tag
        # an element's own 'color' (Adwaita sets junk like #bebebe) would win over the class: drop it
        new = re.sub(r'\scolor="[^"]*"', "", new)
        new = re.sub(r"(style=\"[^\"]*?)(?<![-\w])color:[^;\"]*;?", r"\1", new)
        if re.search(r'\sclass="', new):
            new = re.sub(r'\sclass="([^"]*)"', lambda mm: ' class="%s ColorScheme-Text"' % mm.group(1), new, count=1)
        else:
            new = re.sub(r"^<(\w+)", r'<\1 class="ColorScheme-Text"', new, count=1)
        changed = True
        return new

    out = re.sub(r"<(\w+)\b[^<>]*>", fix_tag, svg)
    if not changed:
        # glyphs without any fill attribute are black by default: let the root carry the colour instead
        out, n = re.subn(r"(<svg\b[^<>]*>)", r'\1<g class="ColorScheme-Text" fill="currentColor">', svg, count=1)
        if not n or "</svg>" not in out:
            return None
        out = out.replace("</svg>", "</g></svg>")
    style = '<style type="text/css" id="current-color-scheme">.ColorScheme-Text{color:%s;}</style>' % default
    out, n = re.subn(r"(<svg\b[^<>]*>)", lambda mm: mm.group(1) + style, out, count=1)
    return out if n else None


def atomic_write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = "%s.new.%d" % (path, os.getpid())
    with open(tmp, "w") as f:
        f.write(data)
    os.replace(tmp, path)


def atomic_symlink(path, target):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if os.path.islink(path) and os.readlink(path) == target:
        return
    tmp = "%s.new.%d" % (path, os.getpid())
    os.symlink(target, tmp)
    os.replace(tmp, path)


def index_theme(name, breeze, dirs):
    s = ["[Icon Theme]", "Name=" + name,
         "Comment=Adwaita symbolic icons that follow the KDE colour scheme (generated from the installed Adwaita icon theme)",
         "Inherits=Adwaita,%s,hicolor" % breeze, "FollowsColorScheme=true", "Example=document-open",
         "DisplayDepth=32", "DesktopDefault=48", "DesktopSizes=16,22,32,48,64,72,96,128",
         "ToolbarDefault=22", "ToolbarSizes=16,22,32,48", "MainToolbarDefault=22", "MainToolbarSizes=16,22,32,48",
         "SmallDefault=16", "SmallSizes=16", "PanelDefault=32", "PanelSizes=16,22,32,48,64,72,96,128", "",
         "Directories=" + ",".join(d for d, _ in dirs), ""]
    for d, ctx in dirs:
        lo, hi = ("16", "32") if d.startswith("symbolic-up-to-32") else ("8", "512")
        s += ["[%s]" % d, "Context=" + ctx, "Size=16", "MinSize=" + lo, "MaxSize=" + hi, "Type=Scalable", ""]
    return "\n".join(s)


CONTEXT = {"actions": "Actions", "apps": "Applications", "categories": "Categories", "devices": "Devices",
           "emblems": "Emblems", "emotes": "Emotes", "legacy": "Legacy", "mimetypes": "MimeTypes", "places": "Places",
           "status": "Status", "ui": "UI"}


def build(src, out):
    # names Adwaita already has as full-colour icons: those stay Adwaita's (no alias that would shadow them)
    colour = set()
    for d in FULLCOLOUR_DIRS:
        p = os.path.join(src, d)
        if os.path.isdir(p):
            colour |= {os.path.splitext(f)[0] for f in os.listdir(p)}
    # all symbolic icons: (dir, base name without -symbolic)
    icons = []
    for kind in ["symbolic"] + ["symbolic-up-to-32"]:
        for ctx in sorted(os.listdir(os.path.join(src, kind))):
            p = os.path.join(src, kind, ctx)
            if not os.path.isdir(p):
                continue
            for f in sorted(os.listdir(p)):
                if f.endswith("-symbolic.svg"):
                    icons.append((kind + "/" + ctx, f[:-len("-symbolic.svg")]))
    total_bytes = n_icons = n_alias = n_skip = 0
    results = {}
    for tname, default, breeze in THEMES:
        root = os.path.join(out, tname)
        keep = set()
        dirs_used = {}
        alias_taken = set()
        have = {}  # (base) -> dir of the symbolic file
        for d, base in icons:
            have.setdefault(base, d)
        for d, base in icons:
            data = open(os.path.join(src, d, base + "-symbolic.svg"), encoding="utf-8", errors="replace").read()
            new = recolour(data, default)
            if new is None:
                new, n_skip = data, n_skip + 1  # no recolourable glyph: copy as is
            dst = os.path.join(root, d, base + "-symbolic.svg")
            atomic_write(dst, new)
            keep.add(dst)
            dirs_used[d] = d.split("/")[1]
            if tname == THEMES[0][0]:
                n_icons += 1
                total_bytes += len(new.encode("utf-8"))
            # plain-name alias
            if base not in colour and base not in alias_taken:
                alias_taken.add(base)
                ad = os.path.join(root, d, base + ".svg")
                atomic_symlink(ad, base + "-symbolic.svg")
                keep.add(ad)
                if tname == THEMES[0][0]:
                    n_alias += 1
        for alias, target in sorted(EXTRA_ALIASES.items()):
            if alias in colour or alias in alias_taken or target not in have:
                continue
            d = have[target]
            ad = os.path.join(root, d, alias + ".svg")
            atomic_symlink(ad, target + "-symbolic.svg")
            keep.add(ad)
            alias_taken.add(alias)
            if tname == THEMES[0][0]:
                n_alias += 1
        order = [d for d, _ in sorted(((k, v) for k, v in dirs_used.items()), key=lambda kv: kv[0])]
        idx = index_theme(tname, breeze, [(d, CONTEXT.get(dirs_used[d], "Actions")) for d in order])
        atomic_write(os.path.join(root, "index.theme"), idx)
        keep.add(os.path.join(root, "index.theme"))
        # drop files of an earlier run that no longer exist (e.g. icons removed in a newer Adwaita)
        for dp, _dn, fn in os.walk(root):
            for f in fn:
                p = os.path.join(dp, f)
                if p not in keep:
                    os.remove(p)
        results[tname] = root
    size = 0
    for root in results.values():
        for dp, _dn, fn in os.walk(root):
            for f in fn:
                p = os.path.join(dp, f)
                if not os.path.islink(p):
                    size += os.path.getsize(p)
    print("Icon themes %s: %d symbolic icons recoloured, %d plain-name aliases, %.0f KiB on disk in %s"
          % (" and ".join(results), n_icons, n_alias, size / 1024, out))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--out", default=os.path.join(os.environ.get("XDG_DATA_HOME") or os.path.expanduser("~/.local/share"), "icons"))
    ap.add_argument("--src")
    ap.add_argument("--remove", action="store_true")
    a = ap.parse_args()
    if a.remove:
        for tname, _d, _b in THEMES:
            shutil.rmtree(os.path.join(a.out, tname), ignore_errors=True)
        return 0
    src = find_adwaita(a.src)
    if not src:
        print("The Adwaita icon theme is not installed, so the Libadwaita icon themes were skipped.\n"
              "Install %s and run this again." % PKG_HINT, file=sys.stderr)
        return 3
    return build(src, a.out)


if __name__ == "__main__":
    sys.exit(main())
