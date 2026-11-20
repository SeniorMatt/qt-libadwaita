// SPDX-License-Identifier: GPL-2.0-or-later
// adwaita-gallery: one widget per libadwaita reference-shot section.
//   adwaita-gallery [--style Libadwaita] [--scale 1|2] [--shot DIR]
// With --shot every section is grabbed to DIR/<section>@<scale>x.png and the app exits.
// Scheme, accent and contrast come from ADWAITA_COLOR_SCHEME / ADWAITA_ACCENT / ADWAITA_HIGH_CONTRAST.
#include <QApplication>
#include <cstdio>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEnterEvent>
#include <QEventLoop>
#include <QFontDatabase>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QPainter>
#include <QGroupBox>
#include <QListWidget>
#include <QSplitter>
#include <QTableWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>

namespace {

// Reference layout (tests/visual/reference-tools/adw_gallery.py): a section is a window-coloured box of at least w x h,
// with 12 px margins, content aligned top-left, 12 px spacing (boxes `pad`, `hbox`, `vbox`).
QWidget *sect(QLayout *l, int w, int h)
{
    auto *p = new QWidget;
    p->setAutoFillBackground(true);
    p->setAttribute(Qt::WA_KeyboardFocusChange, true);  // keyboard-navigation state: focus rings visible
    auto *outer = new QVBoxLayout(p);
    outer->setContentsMargins(12, 12, 12, 12);
    outer->setSpacing(0);
    outer->addLayout(l);
    outer->addStretch();
    p->ensurePolished();
    p->setFixedSize(qMax(w, p->sizeHint().width()), h);  // reference boxes have an exact requested height
    return p;
}
// Horizontal / vertical box, content packed to the start (halign/valign START).
QHBoxLayout *hbox(std::initializer_list<QWidget *> kids, int sp = 12)
{
    auto *l = new QHBoxLayout; l->setContentsMargins(0, 0, 0, 0); l->setSpacing(sp);
    for (auto *k : kids) l->addWidget(k, 0, Qt::AlignTop);
    l->addStretch();
    return l;
}
QVBoxLayout *vbox(std::initializer_list<QLayout *> kids, int sp = 12)
{
    auto *l = new QVBoxLayout; l->setContentsMargins(0, 0, 0, 0); l->setSpacing(sp);
    for (auto *k : kids) l->addLayout(k);
    return l;
}
// Legacy helper for the Qt-only sections.
QWidget *page(QLayout *l, int w = 420)
{
    auto *p = new QWidget;
    p->setAutoFillBackground(true);
    p->setAttribute(Qt::WA_KeyboardFocusChange, true);
    l->setContentsMargins(24, 24, 24, 24);
    l->setSpacing(12);
    p->setLayout(l);
    p->setFixedWidth(w);
    return p;
}

QPushButton *btn(const QString &text, const char *cls = nullptr, bool enabled = true)
{
    auto *b = new QPushButton(text);
    if (cls) b->setProperty("adwaita-class", cls);
    b->setEnabled(enabled);
    return b;
}
QPushButton *btn2(const QString &text, const char *c1, const char *c2, bool enabled = true)
{
    return btn(text, (QByteArray(c1) + ' ' + c2).constData(), enabled);  // several space-separated classes
}

QCheckBox *sw(bool on, bool enabled, const QString &text = {})
{
    auto *c = new QCheckBox(text);
    c->setProperty("adwaita-switch", true);
    c->setChecked(on);
    c->setEnabled(enabled);
    return c;
}

QWidget *buttons()
{
    return sect(vbox({
        hbox({btn("Button"), btn("Suggested", "suggested-action"), btn("Destructive", "destructive-action"),
              btn("Flat", "flat"), btn("Pill", "pill"), btn("Disabled", nullptr, false)}),
        hbox({btn2("Suggested pill", "suggested-action", "pill"), btn("Disabled suggested", "suggested-action", false),
              btn("Disabled flat", "flat", false)})}), 560, 130);
}

QWidget *entries()
{
    auto *a = new QLineEdit; a->setPlaceholderText("Placeholder");
    auto *b = new QLineEdit("Some text");
    auto *c = new QLineEdit("Disabled"); c->setEnabled(false);
    for (auto *e : {a, b, c}) e->setFixedWidth(160);
    return sect(hbox({a, b, c}), 680, 70);
}

QWidget *spinbutton()
{
    auto *s = new QSpinBox; s->setValue(42);
    auto *d = new QSpinBox; d->setValue(7); d->setEnabled(false);
    return sect(hbox({s, d}), 560, 70);
}

QWidget *dropdown()
{
    auto *c = new QComboBox; c->addItems({"First item", "Second item", "Third item"});
    auto *d = new QComboBox; d->addItems({"Disabled item"}); d->setEnabled(false);
    return sect(hbox({c, d}), 560, 70);
}

QWidget *switches()
{
    return sect(hbox({sw(false, true), sw(true, true), sw(false, false), sw(true, false)}, 18), 560, 64);
}

QWidget *checks()
{
    auto mk = [](bool on, bool en) { auto *c = new QCheckBox; c->setChecked(on); c->setEnabled(en); return c; };
    return sect(hbox({mk(false, true), mk(true, true), mk(false, false), mk(true, false)}, 18), 560, 64);
}

QWidget *radios()
{
    auto mk = [](bool on, bool en) { auto *r = new QRadioButton; r->setAutoExclusive(false); r->setChecked(on); r->setEnabled(en); return r; };
    // reference: off, on, disabled off, disabled on
    return sect(hbox({mk(false, true), mk(true, true), mk(false, false), mk(true, false)}, 18), 560, 64);
}

QWidget *slider()
{
    auto *a = new QSlider(Qt::Horizontal); a->setValue(40);
    auto *b = new QSlider(Qt::Horizontal); b->setValue(40); b->setEnabled(false);
    for (auto *s : {a, b}) s->setFixedWidth(240);
    return sect(hbox({a, b}), 560, 70);
}

// 240x140 scrolled window, "Scrollable line N" x60, label margins 12 (reference: Gtk.ScrolledWindow, overlay scrolling).
QScrollArea *scrollArea()
{
    QStringList lines;
    for (int i = 0; i < 60; ++i) lines << QString("Scrollable line %1").arg(i);
    auto *lab = new QLabel(lines.join('\n'));
    lab->setContentsMargins(12, 0, 12, 0);
    lab->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto *sa = new QScrollArea;
    sa->setFrameShape(QFrame::NoFrame);
    sa->setWidget(lab);
    sa->setFixedSize(240, 140);
    sa->viewport()->setAutoFillBackground(false);
    for (auto *b : {sa->verticalScrollBar(), sa->horizontalScrollBar()}) { b->setAutoFillBackground(true); b->setBackgroundRole(QPalette::Window); }  // translucent overlay trough needs an opaque base in an offscreen grab
    lab->setAutoFillBackground(false);
    lab->setProperty("_gallery_label", true);
    return sa;
}
QScrollArea *g_scrollArea = nullptr;
QWidget *scrollbar()
{
    g_scrollArea = scrollArea();
    auto *l = new QHBoxLayout; l->setContentsMargins(0, 0, 0, 0); l->addWidget(g_scrollArea, 0, Qt::AlignTop); l->addStretch();
    return sect(l, 560, 170);
}

QWidget *tabs()
{
    auto *t = new QTabBar;
    t->addTab("First"); t->addTab("Second"); t->addTab("Third");
    t->setExpanding(true);
    t->setDrawBase(true);
    t->setCurrentIndex(1);
    t->setTabsClosable(true);
    // close button only on the selected tab (reference: Adw.TabBar shows it on the selected page)
    for (int i = 0; i < t->count(); ++i)
        if (auto *cb = t->tabButton(i, QTabBar::RightSide)) cb->setVisible(i == 1);
    t->setFixedWidth(480);
    auto *l = new QHBoxLayout; l->setContentsMargins(0, 0, 0, 0); l->addWidget(t, 0, Qt::AlignTop); l->addStretch();
    return sect(l, 560, 71);
}

QToolButton *hbIcon(const char *themeName, const QString &fallback)
{
    auto *b = new QToolButton;
    const QIcon ic = QIcon::fromTheme(themeName);
    if (!ic.isNull()) b->setIcon(ic); else b->setText(fallback);
    b->setProperty("adwaita-class", "flat");
    return b;
}

QWidget *headerbar()
{
    auto *hb = new QWidget;
    hb->setProperty("adwaita-class", "headerbar");
    hb->setFixedSize(536, 47);
    auto *l = new QHBoxLayout(hb);
    l->setContentsMargins(6, 0, 6, 0);
    l->setSpacing(6);
    l->addWidget(hbIcon("list-add-symbolic", "+"));
    l->addStretch();
    auto *tl = new QVBoxLayout; tl->setSpacing(0); tl->setContentsMargins(0, 0, 0, 0);
    auto *title = new QLabel("<b>Title</b>"); title->setAlignment(Qt::AlignCenter);
    auto *sub = new QLabel("Subtitle"); sub->setAlignment(Qt::AlignCenter);
    QFont sf = sub->font(); sf.setPointSizeF(sf.pointSizeF() * 0.82); sub->setFont(sf);
    QPalette sp = sub->palette(); QColor c = sp.color(QPalette::WindowText); c.setAlphaF(0.55); sp.setColor(QPalette::WindowText, c); sub->setPalette(sp);
    tl->addWidget(title); tl->addWidget(sub);
    l->addLayout(tl);
    l->addStretch();
    auto *mb = hbIcon("open-menu-symbolic", "☰");
    auto *m = new QMenu(mb); m->addAction("Item");
    mb->setMenu(m); mb->setPopupMode(QToolButton::InstantPopup);
    l->addWidget(mb);
    { auto *cb = hbIcon("window-close-symbolic", "✕"); cb->setProperty("adwaita-class", "flat circular"); l->addWidget(cb); }  // ":close" decoration layout
    auto *wl = new QHBoxLayout; wl->setContentsMargins(0, 0, 0, 0); wl->addWidget(hb, 0, Qt::AlignTop); wl->addStretch();
    return sect(wl, 560, 72);
}

QMenu *makeMenu()
{
    auto *m = new QMenu;
    auto *first = m->addAction("New Window"); m->addAction("Open…"); m->addAction("Save");
    m->addSeparator();
    m->addAction("Preferences");
    m->ensurePolished();
    m->setActiveAction(first);  // reference popover has the first item in hover state
    return m;
}
QWidget *menu() { return makeMenu(); }

QWidget *boxedlist()
{
    auto *list = new QWidget; list->setProperty("adwaita-class", "boxed-list");
    list->setFixedWidth(536);
    auto *ll = new QVBoxLayout(list); ll->setContentsMargins(1, 1, 1, 1); ll->setSpacing(0);
    int idx = 0;
    auto row = [&](QWidget *content, int h = 55) {
        auto *r = new QWidget; r->setProperty("adwaita-class", "boxed-row"); r->setProperty("adwaita-row-index", idx++);
        auto *rl = new QHBoxLayout(r); rl->setContentsMargins(12, 6, 12, 6);
        rl->addWidget(content); r->setMinimumHeight(h); ll->addWidget(r);
        return rl;
    };
    { auto *t = new QWidget; auto *tl = new QVBoxLayout(t); tl->setContentsMargins(0, 0, 0, 0); tl->setSpacing(0);
      auto *sub = new QLabel("With a subtitle");
      QFont sf = sub->font(); sf.setPointSizeF(sf.pointSizeF() * 0.9); sub->setFont(sf);
      QPalette sp = sub->palette(); QColor c = sp.color(QPalette::WindowText); c.setAlphaF(0.55); sp.setColor(QPalette::WindowText, c); sub->setPalette(sp);
      tl->addWidget(new QLabel("Action row")); tl->addWidget(sub);
      row(t, 55); }
    { auto *rl = row(new QLabel("Switch row")); rl->addStretch(); rl->addWidget(sw(true, true)); }
    { auto *t = new QWidget; auto *tl = new QVBoxLayout(t); tl->setContentsMargins(0, 0, 0, 0); tl->setSpacing(0);
      auto *e = new QLineEdit("Some text"); e->setFrame(false);
      { QPalette ep = e->palette(); ep.setColor(QPalette::Base, Qt::transparent); e->setPalette(ep); }
      auto *tt = new QLabel("Entry row");
      QFont sf = tt->font(); sf.setPointSizeF(sf.pointSizeF() * 0.82); tt->setFont(sf);
      tl->addWidget(tt); tl->addWidget(e);
      row(t, 55); }
    { auto *c = new QComboBox; c->addItems({"One", "Two", "Three"}); c->setProperty("adwaita-class", "flat");
      auto *rl = row(new QLabel("Combo row")); rl->addStretch(); rl->addWidget(c); }
    auto *l = new QHBoxLayout; l->setContentsMargins(0, 0, 0, 0); l->addWidget(list, 0, Qt::AlignTop); l->addStretch();
    return sect(l, 560, 250);
}

class TooltipSample : public QWidget
{
public:
    TooltipSample()
    {
        const QFontMetrics fm(font());
        setFixedSize(fm.horizontalAdvance("A tooltip") + 22, fm.height() + 14);
        setAutoFillBackground(true);
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        QStyleOption o; o.initFrom(this); o.rect = rect();
        style()->drawPrimitive(QStyle::PE_PanelTipLabel, &o, &p, this);
        p.setPen(palette().color(QPalette::ToolTipText));
        p.drawText(o.rect, Qt::AlignCenter, "A tooltip");
    }
};

QWidget *tooltip() { return new TooltipSample; }

QWidget *progress()
{
    auto *p = new QProgressBar; p->setValue(60); p->setFixedWidth(300); p->setTextVisible(false);
    return sect(hbox({p}), 560, 50);
}

QWidget *focusringSection(bool entryFocused)
{
    auto *b = new QPushButton("Focused button");
    auto *e = new QLineEdit("Focused entry");
    auto *p = sect(hbox({b, e}), 560, 70);
    if (entryFocused) QTimer::singleShot(0, e, [e] { e->setFocus(Qt::TabFocusReason); });
    else { b->setProperty("adwaita-focus-visible", true); QTimer::singleShot(0, b, [b] { b->setFocus(Qt::TabFocusReason); }); }
    return p;
}
QWidget *focusring() { return focusringSection(false); }
QWidget *focusringEntry() { return focusringSection(true); }

QWidget *listview()
{
    auto *l = new QVBoxLayout;
    auto *v = new QListWidget;
    for (int i = 1; i <= 6; ++i) v->addItem(QString("List item %1").arg(i));
    v->item(1)->setSelected(true); v->setCurrentRow(1);
    v->setFixedHeight(200);
    l->addWidget(v);
    return page(l, 360);
}

QWidget *treeview()
{
    auto *l = new QVBoxLayout;
    auto *t = new QTreeWidget; t->setColumnCount(2); t->setHeaderLabels({"Name", "Size"});
    for (int i = 0; i < 3; ++i) {
        auto *it = new QTreeWidgetItem(t, {QString("Folder %1").arg(i + 1), "--"});
        for (int j = 0; j < 2; ++j) new QTreeWidgetItem(it, {QString("File %1.%2").arg(i + 1).arg(j + 1), "12 kB"});
        if (i < 2) it->setExpanded(true);
    }
    t->setCurrentItem(t->topLevelItem(0)->child(1));
    t->setSortingEnabled(true);
    t->setFixedHeight(220);
    l->addWidget(t);
    return page(l, 360);
}

QWidget *tableview()
{
    auto *l = new QVBoxLayout;
    auto *t = new QTableWidget(5, 3);
    t->setHorizontalHeaderLabels({"Name", "Type", "Size"});
    for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) t->setItem(r, c, new QTableWidgetItem(QString("Cell %1,%2").arg(r).arg(c)));
    t->setCurrentCell(2, 1);
    t->setFixedHeight(200);
    l->addWidget(t);
    return page(l, 420);
}

QWidget *groupbox()
{
    auto *l = new QVBoxLayout;
    auto *g = new QGroupBox("Group title");
    auto *gl = new QVBoxLayout(g); gl->addWidget(new QCheckBox("Option one")); gl->addWidget(new QCheckBox("Option two"));
    l->addWidget(g);
    auto *g2 = new QGroupBox("Checkable"); g2->setCheckable(true);
    auto *g2l = new QVBoxLayout(g2); g2l->addWidget(new QLabel("Contents"));
    l->addWidget(g2);
    auto *line = new QFrame; line->setFrameShape(QFrame::HLine); l->addWidget(line);
    auto *fr = new QFrame; fr->setFrameShape(QFrame::StyledPanel); auto *fl = new QVBoxLayout(fr); fl->addWidget(new QLabel("Framed"));
    l->addWidget(fr);
    return page(l, 360);
}

QWidget *toolbuttons()
{
    auto *l = new QVBoxLayout;
    auto *tb = new QToolBar; tb->setIconSize(QSize(16, 16));
    tb->addAction(QApplication::style()->standardIcon(QStyle::SP_ArrowBack), "Back");
    tb->addAction(QApplication::style()->standardIcon(QStyle::SP_ArrowForward), "Forward");
    tb->addSeparator();
    auto *a = tb->addAction(QApplication::style()->standardIcon(QStyle::SP_FileIcon), "Checked"); a->setCheckable(true); a->setChecked(true);
    tb->addAction("Text");
    l->addWidget(tb);
    auto *row = new QHBoxLayout;
    auto *tbt = new QToolButton; tbt->setText("Menu"); auto *m = new QMenu(tbt); m->addAction("One"); tbt->setMenu(m);
    tbt->setPopupMode(QToolButton::MenuButtonPopup); row->addWidget(tbt);
    auto *t2 = new QToolButton; t2->setText("Raised"); t2->setProperty("adwaita-class", "raised"); row->addWidget(t2);
    auto *t3 = new QToolButton; t3->setText("Off"); t3->setEnabled(false); row->addWidget(t3);
    row->addStretch();
    l->addLayout(row);
    auto *sp = new QSplitter; sp->addWidget(new QLabel("Left")); sp->addWidget(new QLabel("Right")); l->addWidget(sp);
    return page(l, 420);
}

QWidget *tabwidget()
{
    auto *l = new QVBoxLayout;
    auto *t = new QTabWidget; t->setTabsClosable(true);
    t->addTab(new QLabel("Page one"), "First"); t->addTab(new QLabel("Page two"), "Second"); t->addTab(new QLabel("Page three"), "Third");
    t->setFixedHeight(140);
    l->addWidget(t);
    return page(l, 420);
}

QWidget *combopopup()
{
    auto *c = new QComboBox; c->addItems({"First", "Second", "Third"});
    c->setMinimumWidth(180);
    auto *p = new QWidget; auto *l = new QVBoxLayout(p); l->addWidget(c);
    p->setFixedSize(300, 220); p->setAutoFillBackground(true);
    QTimer::singleShot(0, c, [c] { c->showPopup(); });
    return p;
}

// Post-show steps (run before the grab): wait for the style's animations, or put the pointer over the scroll bar.
void waitMs(int ms)
{
    QEventLoop loop; QTimer::singleShot(ms, &loop, &QEventLoop::quit); loop.exec();
}
void prepScrollIdle(QWidget *) { waitMs(2500); }   // overlay indicator faded out
void prepScrollThin(QWidget *) { g_scrollArea->verticalScrollBar()->setValue(30); waitMs(200); }  // just scrolled: thin indicator
void prepScrollHover(QWidget *)
{
    // Reference: pointer over the scroll bar -> expanded indicator. No real pointer offscreen: send the enter event.
    QScrollBar *sb = g_scrollArea->verticalScrollBar();
    waitMs(100);
    QEnterEvent ev(QPointF(4, 20), QPointF(4, 20), QPointF(4, 20));
    QCoreApplication::sendEvent(sb, &ev);
    waitMs(700);
}

struct Section {
    const char *name;
    std::function<QWidget *()> make;
    std::function<void(QWidget *)> prep = {};
    int crop = 0;  // >0: cut this many px off every side of the grab, <0: pad with window colour (popup shadow margin)
};
const Section kSections[] = {
    {"buttons", buttons}, {"entries", entries}, {"spinbutton", spinbutton}, {"dropdown", dropdown},
    {"switches", switches}, {"checks", checks}, {"radios", radios}, {"slider", slider},
    {"scrollbar", scrollbar, prepScrollHover}, {"scrollbar-idle", scrollbar, prepScrollIdle},
    {"scrollbar-thin", scrollbar, prepScrollThin},
    {"tabs", tabs}, {"headerbar", headerbar}, {"menu", menu, {}, 12}, {"menu-shadow", menu, {}, -12},
    {"boxedlist", boxedlist}, {"tooltip", tooltip}, {"progress", progress},
    {"focusring", focusring}, {"focusring-entry", focusringEntry},
    // Qt-only sections (no libadwaita reference)
    {"listview", listview}, {"treeview", treeview}, {"tableview", tableview}, {"groupbox", groupbox},
    {"toolbuttons", toolbuttons}, {"tabwidget", tabwidget},
};

}  // namespace

int main(int argc, char **argv)
{
    // --scale must be applied before QApplication exists.
    QString shotDir, scale = "1";
    for (int i = 1; i < argc; ++i) {
        const QByteArray a = argv[i];
        if (a == "--scale" && i + 1 < argc) scale = argv[i + 1];
        if (a == "--shot" && i + 1 < argc) shotDir = argv[i + 1];
    }
    if (scale != "1") qputenv("QT_SCALE_FACTOR", scale.toUtf8());
    // Shots are static end states: no widget may be caught half-way through a style transition.
    if (!shotDir.isEmpty() && !qEnvironmentVariableIsSet("ADWAITA_ANIMATIONS")) qputenv("ADWAITA_ANIMATIONS", "0");

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);
    // Reference font: Adwaita Sans 11 (not installed on the host; load from ADWAITA_GALLERY_FONT_DIR, read-only).
    if (const QByteArray fd = qgetenv("ADWAITA_GALLERY_FONT_DIR"); !fd.isEmpty()) {
        const QStringList files = QDir(QString::fromLocal8Bit(fd)).entryList({"*.ttf", "*.otf"}, QDir::Files);
        for (const QString &f : files) QFontDatabase::addApplicationFont(QDir(QString::fromLocal8Bit(fd)).filePath(f));
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QStringList fam = QFontDatabase::families();
#else
        const QStringList fam = QFontDatabase().families();
#endif
        if (fam.contains("Adwaita Sans")) QApplication::setFont(QFont("Adwaita Sans", 11));
        if (fam.contains("Adwaita Mono")) QApplication::setFont(QFont("Adwaita Mono", 11), "QPlainTextEdit");
    }
    QString styleName = "Libadwaita";
    for (int i = 1; i + 1 < argc; ++i)
        if (QByteArray(argv[i]) == "--style") styleName = argv[i + 1];
    QApplication::setStyle(styleName);
    // setStyle() silently keeps the default style if the plugin was not found: report that (and fail in --shot mode).
    if (QApplication::style()->objectName().compare(styleName, Qt::CaseInsensitive) != 0) {
        qWarning("style '%s' was not loaded (got '%s'); available: %s", qPrintable(styleName),
                 qPrintable(QApplication::style()->objectName()), qPrintable(QStyleFactory::keys().join(", ")));
        if (!shotDir.isEmpty()) return 3;
    } else if (!shotDir.isEmpty()) {
        std::fprintf(stderr, "style loaded: %s\n", qPrintable(styleName));
    }

    if (!shotDir.isEmpty()) {
        QDir().mkpath(shotDir);
        for (const Section &s : kSections) {
            QWidget *w = s.make();
            w->setAttribute(Qt::WA_KeyboardFocusChange, true);
            w->show();
            app.processEvents();
            app.processEvents();
            // Only the focusring sections show a focus ring; show() autofocuses the first entry elsewhere.
            if (const QByteArray n = s.name; n != "focusring" && n != "focusring-entry")
                if (QWidget *f = QApplication::focusWidget()) { f->clearFocus(); app.processEvents(); }
            if (s.prep) s.prep(w);
            const QString path = QString("%1/%2@%3x.png").arg(shotDir, s.name, scale);
            // Translucent popups (menu) grab with an alpha channel; flatten onto the window colour so viewers
            // that drop alpha (contact sheets) do not show a black rectangle around the rounded panel.
            const qreal dpr = w->devicePixelRatioF();
            QImage img((w->size() * dpr).expandedTo(QSize(1, 1)), QImage::Format_RGB32);
            img.setDevicePixelRatio(dpr);
            img.fill(QApplication::palette().color(QPalette::Window));
            w->render(&img);  // render onto an opaque base: translucent parts (scroll trough, popup shadow) blend like on screen
            if (s.crop > 0) {
                const qreal r = img.devicePixelRatio(); const int m = qRound(s.crop * r);
                QImage c = img.copy(m, m, img.width() - 2 * m, img.height() - 2 * m); c.setDevicePixelRatio(r); img = c;
            } else if (s.crop < 0) {
                const qreal r = img.devicePixelRatio(); const int m = qRound(-s.crop * r);
                QImage c(img.width() + 2 * m, img.height() + 2 * m, QImage::Format_RGB32);
                c.fill(QApplication::palette().color(QPalette::Window));
                { QPainter cp(&c); cp.drawImage(m, m, img); }
                c.setDevicePixelRatio(r); img = c;
            }
            if (!img.save(path)) qWarning("failed to write %s", qPrintable(path));
            delete w;
        }
        return 0;
    }

    // Interactive: all sections in a scrolling column.
    auto *all = new QWidget;
    auto *v = new QVBoxLayout(all);
    for (const Section &s : kSections) {
        if (QByteArray(s.name) == "menu" || QByteArray(s.name) == "menu-shadow" || QByteArray(s.name) == "tooltip") continue;
        v->addWidget(new QLabel(QString("<b>%1</b>").arg(s.name)));
        v->addWidget(s.make());
    }
    auto *sa = new QScrollArea; sa->setWidget(all); sa->setWidgetResizable(true); sa->resize(520, 800);
    sa->setWindowTitle("Adwaita gallery");
    sa->show();
    return app.exec();
}
