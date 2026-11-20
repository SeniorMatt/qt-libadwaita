#!/usr/bin/env python3
"""Run a host Qt app in a private Xvfb with the Libadwaita style from ~/.local and a throwaway config dir, screenshot it.
usage: shoot_host_app.py COMMAND WINDOW_CLASS_REGEX OUT.png [light|dark] [args...]   env: SHOT_KEYS, SHOT_CONFIG_DIR"""
import os, subprocess, sys, tempfile, time
cmd0, name, out = sys.argv[1:4]
scheme = sys.argv[4] if len(sys.argv) > 4 else "light"
extra = sys.argv[5:]
disp = ":77"
cfg = os.environ.get("SHOT_CONFIG_DIR") or tempfile.mkdtemp(prefix="shot-cfg-")
xv = subprocess.Popen(["Xvfb", disp, "-screen", "0", "1400x900x24", "-nolisten", "tcp"], stderr=subprocess.DEVNULL)
time.sleep(1.5)
env = dict(os.environ, DISPLAY=disp, QT_PLUGIN_PATH=os.path.expanduser("~/.local/lib/qt6/plugins") + ":" + os.path.expanduser("~/.local/lib/qt5/plugins"),
           QT_STYLE_OVERRIDE="Libadwaita", QT_QPA_PLATFORM="xcb", ADWAITA_COLOR_SCHEME=scheme, XDG_CONFIG_HOME=cfg,
           LANG="C.UTF-8")
p = subprocess.Popen([cmd0] + extra, env=env, stdout=subprocess.DEVNULL, stderr=open(out + ".log", "w"))
wid = None
for _ in range(60):
    time.sleep(1)
    r = subprocess.run(["xdotool", "search", "--onlyvisible", "--class", name], env=env, capture_output=True, text=True)
    if r.stdout.strip(): wid = r.stdout.split()[-1]; break
if wid:
    subprocess.run(["xdotool", "windowsize", wid, "1100", "700"], env=env)
    subprocess.run(["xdotool", "windowmove", wid, "0", "0"], env=env)
    time.sleep(5)
    for k in os.environ.get("SHOT_KEYS", "").split():
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
p.terminate()
try: p.wait(timeout=15)
except subprocess.TimeoutExpired: p.kill(); p.wait()
xv.terminate(); xv.wait()
