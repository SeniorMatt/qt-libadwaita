#!/usr/bin/env python3
"""Compose the README images from the gallery shots (run tools/shoot_qt.sh or tools/compare/run.sh first)."""
import os
from PIL import Image, ImageDraw
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
shots = os.path.join(root, "docs/qt-shots")
out = os.path.join(root, "docs/readme")
os.makedirs(out, exist_ok=True)
PAD = 40

def column(variant, accent, names):
    ims = [Image.open(f"{shots}/{variant}/{accent}/{n}@2x.png").convert("RGB") for n in names]
    bg = ims[0].getpixel((ims[0].width - 2, ims[0].height // 2))
    w = max(i.width for i in ims) + 2 * PAD
    h = sum(i.height for i in ims) + PAD * (len(ims) + 1)
    col = Image.new("RGB", (w, h), bg)
    y = PAD
    for i in ims:
        col.paste(i, (PAD, y)); y += i.height + PAD
    return col

def pair(a, b, name):
    h = max(a.height, b.height)
    img = Image.new("RGB", (a.width + b.width, h), a.getpixel((2, 2)))
    img.paste(a, (0, 0)); img.paste(b.resize(b.size), (a.width, 0))
    if b.height < h: ImageDraw.Draw(img).rectangle((a.width, b.height, img.width, h), fill=b.getpixel((2, 2)))
    img = img.resize((img.width // 2, img.height // 2), Image.LANCZOS)
    img.save(os.path.join(out, name), optimize=True)
    print("wrote", name, img.size)

hero = ["headerbar", "buttons", "entries", "switches", "checks", "radios", "slider", "progress", "tabs"]
pair(column("light", "blue", hero), column("dark", "blue", hero), "hero.png")
lists = ["boxedlist", "listview"]
pair(column("light", "blue", lists), column("dark", "blue", lists), "lists.png")
pair(column("dark", "blue", ["buttons", "switches"]), column("dark", "orange", ["buttons", "switches"]), "accents.png")
pair(column("hc-light", "blue", ["buttons", "entries", "checks"]), column("hc-dark", "blue", ["buttons", "entries", "checks"]), "high-contrast.png")
