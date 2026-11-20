#!/usr/bin/env python3
"""Run a Flatpak Qt app in a private Xvfb with the Libadwaita style from ~/.local and screenshot it.
usage: shoot_flatpak_app.py APP_ID WINDOW_CLASS_REGEX OUT.png [light|dark] [extra app args...]"""
import os, subprocess, sys, time
from PIL import Image
app, name, out = sys.argv[1:4]
scheme = sys.argv[4] if len(sys.argv) > 4 else "light"
extra = sys.argv[5:]
# KDE apps paint many areas from KColorScheme (kdeglobals), so they also need the matching KDE colour scheme.
# Flatpak forces XDG_CONFIG_HOME to ~/.var/app/<id>/config: put kdeglobals there for this run only.
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
kg = os.path.expanduser(f"~/.var/app/{app}/config/kdeglobals")
if os.path.exists(kg) and not os.path.exists(kg + ".qt-libadwaita-test"):
    sys.exit(f"{kg} exists and is not ours; not touching it")
os.makedirs(os.path.dirname(kg), exist_ok=True)
import shutil
shutil.copy(os.path.join(root, "kde-colors", {"light": "Libadwaita", "dark": "LibadwaitaDark"}[scheme] + ".colors"), kg)
open(kg + ".qt-libadwaita-test", "w").close()
disp = ":77"
plug = os.path.expanduser("~/.local/lib/qt6/plugins")
fonts = os.path.expanduser("~/.local/share/fonts")
xv = subprocess.Popen(["Xvfb", disp, "-screen", "0", "1400x900x24", "-nolisten", "tcp"], stderr=subprocess.DEVNULL)
time.sleep(1.5)
env = dict(os.environ, DISPLAY=disp)
cmd = ["flatpak", "run", "--user", f"--filesystem={plug}:ro", f"--filesystem={fonts}:ro", "--nosocket=wayland",
       f"--env=QT_PLUGIN_PATH={plug}", "--env=QT_STYLE_OVERRIDE=" + os.environ.get("SHOT_STYLE", "Libadwaita"), "--env=QT_QPA_PLATFORM=xcb", "--env=LANG=C.UTF-8", "--env=XDG_CURRENT_DESKTOP=KDE", "--env=KDE_FULL_SESSION=true", "--env=KDE_SESSION_VERSION=6"] + \
       ([f"--env=ADWAITA_COLOR_SCHEME={scheme}"] if not os.environ.get("SHOT_NO_SCHEME_ENV") else []) + [f"--env={e}" for e in os.environ.get("SHOT_ENV", "").split()] + [app] + extra
p = subprocess.Popen(cmd, env=env, stdout=subprocess.DEVNULL, stderr=open(out + ".log", "w"))
wid = None
for _ in range(60):
    time.sleep(1)
    r = subprocess.run(["xdotool", "search", "--onlyvisible", "--class", name], env=env, capture_output=True, text=True)
    if r.stdout.strip():
        wid = r.stdout.split()[-1]; break
if wid:
    subprocess.run(["xdotool", "windowsize", wid, "1100", "700"], env=env)
    subprocess.run(["xdotool", "windowmove", wid, "0", "0"], env=env)
    time.sleep(4)
    for k in os.environ.get("SHOT_KEYS", "").split():  # e.g. SHOT_KEYS=alt+f opens the File menu
        if k.startswith("click:"):  # click:X,Y clicks there (no WM means no keyboard focus for alt+key)
            x, y = k[6:].split(",")
            subprocess.run(["xdotool", "mousemove", x, y, "click", "1"], env=env)
        else:
            subprocess.run(["xdotool", "key", k], env=env)
        time.sleep(1.5)
    if os.path.exists(out): os.remove(out)
    subprocess.run(["ffmpeg", "-loglevel", "error", "-f", "x11grab", "-video_size", "1100x700", "-i", disp + "+0,0",
                    "-frames:v", "1", out], env=env)
    print("saved" if os.path.exists(out) else "FAILED", out)
else:
    print("no window found for", name)
subprocess.run(["flatpak", "kill", app])
p.wait(timeout=20)
xv.terminate(); xv.wait()
os.remove(kg); os.remove(kg + ".qt-libadwaita-test")
