#!/usr/bin/env python3
"""libadwaita reference gallery: builds fixed-size widget sections and saves each as a
PNG at real device pixels (Gtk.WidgetPaintable -> GskRenderer.render_texture).

Run inside the GNOME 51 Flatpak runtime (see shoot_all.sh).
Variant is taken from the environment (ADW_DEBUG_COLOR_SCHEME, ADW_DEBUG_HIGH_CONTRAST,
ADW_DEBUG_ACCENT_COLOR, GDK_SCALE).  Usage: adw_gallery.py OUTDIR [--info]
"""
import ctypes, os, sys
import gi
gi.require_version("Gtk", "4.0"); gi.require_version("Adw", "1")
gi.require_version("Gdk", "4.0"); gi.require_version("Graphene", "1.0")
gi.require_version("GdkX11", "4.0")
from gi.repository import Gtk, Adw, Gdk, GLib, Gio, Graphene, GdkX11

OUT = sys.argv[1]
SCALE = int(os.environ.get("GDK_SCALE", "1"))
FONT = os.environ.get('ADW_REF_FONT', 'Adwaita Sans 11')
W = 560  # section width (logical px)
x11 = ctypes.CDLL("libX11.so.6")
x11.XOpenDisplay.restype = ctypes.c_void_p
_dpy = x11.XOpenDisplay(None)


def warp(x, y):  # device pixels, root coordinates
    root = x11.XDefaultRootWindow(ctypes.c_void_p(_dpy))
    x11.XWarpPointer(ctypes.c_void_p(_dpy), 0, ctypes.c_ulong(root), 0, 0, 0, 0, int(x), int(y))
    x11.XFlush(ctypes.c_void_p(_dpy))


class Tooltip(Gtk.Box):
    """Same CSS node layout as GtkTooltipWindow: tooltip.background > box > label."""
    __gtype_name__ = "RefTooltip"
Tooltip.set_css_name("tooltip")


def pad(child, w, h, name):
    box = Gtk.Box(); box.add_css_class("background")
    box.set_size_request(w, h)
    child.set_margin_start(12); child.set_margin_end(12)
    child.set_margin_top(12); child.set_margin_bottom(12)
    child.set_halign(Gtk.Align.START); child.set_valign(Gtk.Align.START)
    box.append(child); box.sect = name
    return box


def hbox(*kids, sp=12):
    b = Gtk.Box(spacing=sp)
    for k in kids: b.append(k)
    return b


def vbox(*kids, sp=12):
    b = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=sp)
    for k in kids: b.append(k)
    return b


def btn(label, *cls, sens=True):
    b = Gtk.Button(label=label)
    for c in cls: b.add_css_class(c)
    b.set_sensitive(sens); b.set_valign(Gtk.Align.START); return b


def build_sections(win_ref):
    S = {}
    S["buttons"] = pad(vbox(
        hbox(btn("Button"), btn("Suggested", "suggested-action"),
             btn("Destructive", "destructive-action"), btn("Flat", "flat"),
             btn("Pill", "pill"), btn("Disabled", sens=False)),
        hbox(btn("Suggested pill", "suggested-action", "pill"),
             btn("Disabled suggested", "suggested-action", sens=False),
             btn("Disabled flat", "flat", sens=False)),
        ), W, 130, "buttons")

    e1 = Gtk.Entry(placeholder_text="Placeholder"); e2 = Gtk.Entry(text="Some text")
    e3 = Gtk.Entry(text="Disabled", sensitive=False)
    S["entries"] = pad(hbox(e1, e2, e3, sp=12), W + 120, 70, "entries")
    for e in (e1, e2, e3): e.set_size_request(160, -1); e.set_valign(Gtk.Align.START)
    sp = Gtk.SpinButton.new_with_range(0, 100, 1); sp.set_value(42); sp.set_valign(Gtk.Align.START)
    sp2 = Gtk.SpinButton.new_with_range(0, 100, 1); sp2.set_value(7); sp2.set_sensitive(False)
    S["spinbutton"] = pad(hbox(sp, sp2), W, 70, "spinbutton")
    dd = Gtk.DropDown.new_from_strings(["First item", "Second item", "Third item"])
    dd2 = Gtk.DropDown.new_from_strings(["Disabled item"]); dd2.set_sensitive(False)
    S["dropdown"] = pad(hbox(dd, dd2), W, 70, "dropdown")

    def sw(a, s=True):
        w = Gtk.Switch(active=a, sensitive=s); w.set_valign(Gtk.Align.CENTER); return w
    S["switches"] = pad(hbox(sw(False), sw(True), sw(False, False), sw(True, False), sp=18), W, 64, "switches")

    def check(a, s=True): return Gtk.CheckButton(active=a, sensitive=s, label="")
    S["checks"] = pad(hbox(check(False), check(True), check(False, False), check(True, False), sp=18), W, 64, "checks")
    r1 = Gtk.CheckButton(); r2 = Gtk.CheckButton(group=r1); r2.set_active(True)
    r3 = Gtk.CheckButton(group=r1); r3.set_sensitive(False)
    r4 = Gtk.CheckButton(); r4.set_sensitive(False)
    r5 = Gtk.CheckButton(group=r4); r5.set_active(True); r5.set_sensitive(False)
    S["radios"] = pad(hbox(r1, r2, r3, r5, sp=18), W, 64, "radios")

    s1 = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, 0, 100, 1); s1.set_value(40)
    s2 = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, 0, 100, 1); s2.set_value(40); s2.set_sensitive(False)
    for s in (s1, s2): s.set_size_request(240, -1); s.set_draw_value(False)
    S["slider"] = pad(hbox(s1, s2), W, 70, "slider")

    tv = Gtk.Label(label="\n".join(f"Scrollable line {i}" for i in range(60)), xalign=0)
    tv.set_margin_start(12); tv.set_margin_end(12)
    sc = Gtk.ScrolledWindow(); sc.set_child(tv); sc.set_size_request(240, 140)
    sc.set_overlay_scrolling(True); sc.set_has_frame = None
    S["scrollbar"] = pad(sc, W, 170, "scrollbar")
    S["scrollbar"].scrolled = sc

    tv_ = Adw.TabView()
    for t in ("First", "Second", "Third"):
        tv_.append(Gtk.Label(label=t)).set_title(t)
    tb = Adw.TabBar(view=tv_, autohide=False); tb.set_size_request(480, -1)
    tv_.set_selected_page(tv_.get_nth_page(1))
    S["tabs"] = pad(tb, W, 70, "tabs")
    S["tabs"].keep = tv_

    hb = Adw.HeaderBar(); hb.set_decoration_layout(":close")
    hb.set_title_widget(Adw.WindowTitle(title="Title", subtitle="Subtitle"))
    hb.pack_start(Gtk.Button(icon_name="list-add-symbolic"))
    mb0 = Gtk.MenuButton(icon_name="open-menu-symbolic", menu_model=Gio.Menu()); hb.pack_end(mb0)
    hb.set_size_request(W - 24, -1)
    S["headerbar"] = pad(hb, W, 72, "headerbar")

    menu = Gio.Menu()
    sec = Gio.Menu(); [sec.append(n, "app.none") for n in ("New Window", "Open…", "Save")]
    menu.append_section(None, sec)
    sec2 = Gio.Menu(); sec2.append("Preferences", "app.none"); menu.append_section(None, sec2)
    mb = Gtk.MenuButton(label="Menu"); pm = Gtk.PopoverMenu.new_from_model(menu); mb.set_popover(pm)
    S["menu"] = pad(mb, W, 260, "menu"); S["menu"].pm = pm; S["menu"].mb = mb

    lb = Gtk.ListBox(); lb.add_css_class("boxed-list"); lb.set_selection_mode(Gtk.SelectionMode.NONE)
    lb.set_size_request(W - 24, -1)
    ar = Adw.ActionRow(title="Action row", subtitle="With a subtitle"); lb.append(ar)
    sr = Adw.SwitchRow(title="Switch row", active=True); lb.append(sr)
    er = Adw.EntryRow(title="Entry row"); er.set_text("Some text"); lb.append(er)
    cr = Adw.ComboRow(title="Combo row", model=Gtk.StringList.new(["One", "Two", "Three"])); lb.append(cr)
    S["boxedlist"] = pad(lb, W, 250, "boxedlist")

    tt = Tooltip(); tt.add_css_class("background")
    inner = Gtk.Box(); inner.append(Gtk.Label(label="A tooltip")); tt.append(inner)
    S["tooltip"] = pad(tt, W, 90, "tooltip")

    pb = Gtk.ProgressBar(fraction=0.6); pb.set_size_request(300, -1)
    S["progress"] = pad(pb, W, 50, "progress")

    fb = Gtk.Button(label="Focused button"); fe = Gtk.Entry(text="Focused entry")
    S["focusring"] = pad(hbox(fb, fe), W, 70, "focusring"); S["focusring"].fb = fb; S["focusring"].fe = fe
    return S


class App(Adw.Application):
    def __init__(self):
        super().__init__(application_id="org.example.AdwGallery",
                         flags=Gio.ApplicationFlags.NON_UNIQUE)
        self.connect("activate", self.on_activate)

    def on_activate(self, app):
        st = Gtk.Settings.get_default()
        st.set_property("gtk-font-name", FONT)
        st.set_property("gtk-xft-hintstyle", "hintslight"); st.set_property("gtk-xft-rgba", "none")  # GNOME defaults
        self.info(st)
        act = Gio.SimpleAction.new("none", None); app.add_action(act)
        self.win = win = Adw.ApplicationWindow(application=app)
        fixed = Gtk.Fixed(); win.set_content(fixed)
        self.S = S = build_sections(win)
        y = 0; self.pos = {}
        for n, w in S.items():
            fixed.put(w, 0, y); self.pos[n] = y
            y += w.get_size_request()[1] + 10
        self.H = y
        win.set_default_size(W + 130, y)
        win.set_resizable(False)
        win.set_focus_visible(True)
        win.present()
        GLib.timeout_add(1500, lambda: (self.step(self.steps()) , False)[1])

    def step(self, gen):
        try: ms = next(gen)
        except StopIteration: self.quit(); return
        GLib.timeout_add(ms, lambda: (self.step(gen), False)[1])

    def info(self, st):
        sm = Adw.StyleManager.get_default()
        lines = [f"libadwaita {Adw.MAJOR_VERSION}.{Adw.MINOR_VERSION}.{Adw.MICRO_VERSION}",
                 f"gtk {Gtk.get_major_version()}.{Gtk.get_minor_version()}.{Gtk.get_micro_version()}",
                 f"gtk-font-name(set) {st.get_property('gtk-font-name')}",
                 f"gtk-icon-theme {st.get_property('gtk-icon-theme-name')}",
                 f"gtk-theme {st.get_property('gtk-theme-name')}",
                 f"xft-antialias {st.get_property('gtk-xft-antialias')} hintstyle {st.get_property('gtk-xft-hintstyle')} rgba {st.get_property('gtk-xft-rgba')}",
                 f"dark {sm.get_dark()} high_contrast {sm.get_high_contrast()} accent {sm.get_accent_color().value_nick}",
                 f"scale {SCALE}"]
        print("\n".join(lines), flush=True)
        open(os.path.join(OUT, "info.txt"), "w").write("\n".join(lines) + "\n")

    def pango_info(self):
        l = Gtk.Label(label="x"); self.win.get_content().put(l, 0, 0) if False else None
        d = l.create_pango_context().get_font_description().to_string()
        return d

    def focus_win(self):
        xid = self.win.get_surface().get_xid()
        x11.XSetInputFocus(ctypes.c_void_p(_dpy), ctypes.c_ulong(xid), 1, 0); x11.XFlush(ctypes.c_void_p(_dpy))

    def shot(self, widget, name, w=None, h=None):
        w = w or widget.get_width(); h = h or widget.get_height()
        p = Gtk.WidgetPaintable.new(widget)
        snap = Gtk.Snapshot(); snap.scale(SCALE, SCALE)
        p.snapshot(snap, widget.get_width(), widget.get_height())
        node = snap.to_node()
        r = self.win.get_native().get_renderer()
        tex = r.render_texture(node, Graphene.Rect().init(0, 0, w * SCALE, h * SCALE))
        path = f"{OUT}/{name}@{SCALE}x.png"
        tex.save_to_png(path); print("saved", path, tex.get_width(), tex.get_height(), flush=True)

    def pointer_to(self, widget, dx=0, dy=0):
        ok, pt = widget.compute_point(self.win, Graphene.Point().init(dx, dy)) \
            if False else widget.compute_point(self.win.get_content(), Graphene.Point().init(dx, dy))
        # window assumed at X11 root (0,0) (no window manager under Xvfb)
        warp(pt.x * SCALE, pt.y * SCALE)

    def steps(self):
        S = self.S
        self.focus_win(); self.win.get_root(); warp(self.W_far(), self.H * SCALE - 2)
        yield 600
        for n in ("buttons entries spinbutton dropdown switches checks radios slider tabs headerbar "
                  "boxedlist progress").split():
            self.shot(S[n], n)
        # focus rings
        fr = S["focusring"]; fr.fb.grab_focus(); self.win.set_focus_visible(True); yield 400
        self.shot(fr, "focusring")  # button focused
        fr.fe.grab_focus(); yield 400
        self.shot(fr, "focusring-entry")
        self.win.set_focus(None); yield 200
        # scrollbar: idle, then hovered (pointer over the scrollbar)
        sc = S["scrollbar"].scrolled
        self.shot(S["scrollbar"], "scrollbar-idle")
        adj = sc.get_vadjustment(); adj.set_value(30); yield 150
        self.shot(S["scrollbar"], "scrollbar-thin"); yield 200
        adj.set_value(0)
        self.pointer_to(sc, 236, 70); yield 300
        self.pointer_to(sc, 237, 71); yield 700
        self.shot(S["scrollbar"], "scrollbar")
        warp(self.W_far(), self.H * SCALE - 2); yield 500
        # hover state of button for reference
        self.shot(S["tooltip"].get_first_child(), "tooltip")
        # menu popover
        m = S["menu"]; m.mb.popup(); yield 1200
        pop = m.pm
        self.shot(pop, "menu")
        self.shot_margin(pop, "menu-shadow", 24)
        m.mb.popdown(); yield 300

    def W_far(self): return (W + 125) * SCALE

    def shot_margin(self, widget, name, m):
        p = Gtk.WidgetPaintable.new(widget)
        snap = Gtk.Snapshot(); snap.scale(SCALE, SCALE)
        p.snapshot(snap, widget.get_width(), widget.get_height())
        r = self.win.get_native().get_renderer()
        tex = r.render_texture(snap.to_node(), Graphene.Rect().init(-m*SCALE, -m*SCALE, (widget.get_width()+2*m)*SCALE, (widget.get_height()+2*m)*SCALE))
        tex.save_to_png(f"{OUT}/{name}@{SCALE}x.png")


App().run([sys.argv[0]])
