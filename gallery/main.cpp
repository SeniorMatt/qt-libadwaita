// SPDX-License-Identifier: GPL-2.0-or-later
// adwaita-gallery: one widget per libadwaita reference-shot section.
//   adwaita-gallery [--style Libadwaita] [--scale 1|2] [--shot DIR]
// With --shot every section is grabbed to DIR/<section>@<scale>x.png and the app exits.
// Scheme, accent and contrast come from ADWAITA_COLOR_SCHEME / ADWAITA_ACCENT / ADWAITA_HIGH_CONTRAST.
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
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
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
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

QWidget *page(QLayout *l, int w = 420)
{
    auto *p = new QWidget;
    p->setAutoFillBackground(true);
    p->setAttribute(Qt::WA_KeyboardFocusChange, true);  // keyboard-navigation state: focus rings visible
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
    auto *l = new QVBoxLayout;
    auto *row = new QHBoxLayout;
    row->addWidget(btn("Button"));
    row->addWidget(btn("Suggested", "suggested-action"));
    row->addWidget(btn("Destructive", "destructive-action"));
    l->addLayout(row);
    auto *row2 = new QHBoxLayout;
    row2->addWidget(btn("Flat", "flat"));
    row2->addWidget(btn("Pill", "pill"));
    row2->addWidget(btn("Disabled", nullptr, false));
    l->addLayout(row2);
    return page(l, 460);
}

QWidget *entries()
{
    auto *l = new QVBoxLayout;
    auto *a = new QLineEdit; a->setPlaceholderText("Placeholder");
    auto *b = new QLineEdit("Some text");
    auto *c = new QLineEdit("Disabled"); c->setEnabled(false);
    l->addWidget(a); l->addWidget(b); l->addWidget(c);
    return page(l);
}

QWidget *spinbutton()
{
    auto *l = new QVBoxLayout;
    auto *s = new QSpinBox; s->setValue(42);
    auto *d = new QSpinBox; d->setValue(7); d->setEnabled(false);
    l->addWidget(s); l->addWidget(d);
    return page(l, 280);
}

QWidget *dropdown()
{
    auto *l = new QVBoxLayout;
    auto *c = new QComboBox; c->addItems({"First", "Second", "Third"});
    l->addWidget(c);
    return page(l, 280);
}

QWidget *switches()
{
    auto *l = new QVBoxLayout;
    l->addWidget(sw(false, true, "Off"));
    l->addWidget(sw(true, true, "On"));
    l->addWidget(sw(false, false, "Disabled off"));
    l->addWidget(sw(true, false, "Disabled on"));
    return page(l, 280);
}

QWidget *checks()
{
    auto *l = new QVBoxLayout;
    auto *a = new QCheckBox("Unchecked");
    auto *b = new QCheckBox("Checked"); b->setChecked(true);
    auto *c = new QCheckBox("Disabled"); c->setEnabled(false);
    auto *d = new QCheckBox("Disabled checked"); d->setChecked(true); d->setEnabled(false);
    for (auto *x : {a, b, c, d}) l->addWidget(x);
    return page(l, 280);
}

QWidget *radios()
{
    auto *l = new QVBoxLayout;
    auto *a = new QRadioButton("Off");
    auto *b = new QRadioButton("On"); b->setChecked(true);
    auto *c = new QRadioButton("Disabled"); c->setEnabled(false);
    auto *d = new QRadioButton("Disabled on"); d->setChecked(true); d->setEnabled(false);
    for (auto *x : {a, b, c, d}) l->addWidget(x);
    return page(l, 280);
}

QWidget *slider()
{
    auto *l = new QVBoxLayout;
    auto *a = new QSlider(Qt::Horizontal); a->setValue(40);
    auto *b = new QSlider(Qt::Horizontal); b->setValue(40); b->setEnabled(false);
    l->addWidget(a); l->addWidget(b);
    return page(l);
}

QWidget *scrollbar()
{
    auto *l = new QVBoxLayout;
    auto *sa = new QScrollArea;
    auto *inner = new QWidget;
    auto *il = new QVBoxLayout(inner);
    for (int i = 1; i <= 30; ++i) il->addWidget(new QLabel(QString("Row %1").arg(i)));
    sa->setWidget(inner);
    sa->setFixedHeight(180);
    l->addWidget(sa);
    return page(l);
}

QWidget *tabs()
{
    auto *l = new QVBoxLayout;
    auto *t = new QTabBar;
    t->addTab("First"); t->addTab("Second"); t->addTab("Third");
    l->addWidget(t);
    l->addStretch();
    return page(l);
}

QWidget *headerbar()
{
    auto *hb = new QWidget;
    hb->setProperty("adwaita-class", "headerbar");
    hb->setFixedHeight(47);
    auto *l = new QHBoxLayout(hb);
    l->setContentsMargins(6, 0, 6, 0);
    l->addWidget(btn("Back", "flat"));
    l->addStretch();
    auto *title = new QLabel("<b>Title</b>");
    l->addWidget(title);
    l->addStretch();
    auto *mb = new QToolButton; mb->setText("☰");
    auto *m = new QMenu(mb); m->addAction("Item");
    mb->setMenu(m); mb->setPopupMode(QToolButton::InstantPopup);
    mb->setProperty("adwaita-class", "flat");
    l->addWidget(mb);
    auto *root = new QVBoxLayout;
    root->addWidget(hb);
    root->addStretch();
    auto *p = new QWidget;
    root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);
    p->setLayout(root); p->setAutoFillBackground(true); p->setFixedSize(460, 140);
    return p;
}

QWidget *menu()
{
    auto *m = new QMenu;
    m->addAction("First item"); m->addAction("Second item");
    m->addSeparator();
    m->addAction("Third item"); m->addAction("Fourth item");
    m->ensurePolished();
    return m;
}

QWidget *boxedlist()
{
    auto *l = new QVBoxLayout;
    auto *list = new QWidget; list->setProperty("adwaita-class", "boxed-list");
    auto *ll = new QVBoxLayout(list); ll->setContentsMargins(1, 1, 1, 1); ll->setSpacing(0);
    int idx = 0;
    auto row = [&](QWidget *content) {
        auto *r = new QWidget; r->setProperty("adwaita-class", "boxed-row"); r->setProperty("adwaita-row-index", idx++);
        auto *rl = new QHBoxLayout(r); rl->setContentsMargins(12, 6, 12, 6);
        rl->addWidget(content); r->setMinimumHeight(50); ll->addWidget(r);
        return rl;
    };
    { auto *b = new QPushButton("Activate"); b->setProperty("adwaita-class", "flat");
      auto *rl = row(new QLabel("Action Row")); rl->addStretch(); rl->addWidget(b); }
    { auto *rl = row(new QLabel("Switch Row")); rl->addStretch(); rl->addWidget(sw(true, true)); }
    { auto *rl = row(new QLabel("Entry Row")); auto *e = new QLineEdit("Text"); rl->addWidget(e, 1); }
    { auto *c = new QComboBox; c->addItems({"One", "Two", "Three"});
      auto *rl = row(new QLabel("Combo Row")); rl->addStretch(); rl->addWidget(c); }
    l->addWidget(list);
    return page(l, 460);
}

class TooltipSample : public QWidget
{
public:
    TooltipSample() { setFixedSize(220, 74); setAutoFillBackground(true); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        QStyleOption o; o.initFrom(this); o.rect = QRect(24, 20, 140, 32);
        style()->drawPrimitive(QStyle::PE_PanelTipLabel, &o, &p, this);
        p.setPen(palette().color(QPalette::ToolTipText));
        p.drawText(o.rect, Qt::AlignCenter, "A tooltip");
    }
};

QWidget *tooltip() { return new TooltipSample; }

QWidget *progress()
{
    auto *l = new QVBoxLayout;
    auto *p = new QProgressBar; p->setValue(60);
    l->addWidget(p);
    return page(l);
}

QWidget *focusring()
{
    auto *l = new QVBoxLayout;
    auto *b = btn("Button"); b->setProperty("adwaita-focus-visible", true);
    auto *e = new QLineEdit("Entry");
    l->addWidget(b); l->addWidget(e);
    auto *p = page(l, 320);
    QTimer::singleShot(0, e, [e, b] { e->setFocus(); b->update(); });
    return p;
}

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

struct Section { const char *name; std::function<QWidget *()> make; };
const Section kSections[] = {
    {"buttons", buttons}, {"entries", entries}, {"spinbutton", spinbutton}, {"dropdown", dropdown},
    {"switches", switches}, {"checks", checks}, {"radios", radios}, {"slider", slider},
    {"scrollbar", scrollbar}, {"tabs", tabs}, {"headerbar", headerbar}, {"menu", menu},
    {"boxedlist", boxedlist}, {"tooltip", tooltip}, {"progress", progress}, {"focusring", focusring},
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

    QApplication app(argc, argv);
    QString styleName = "Libadwaita";
    for (int i = 1; i + 1 < argc; ++i)
        if (QByteArray(argv[i]) == "--style") styleName = argv[i + 1];
    QApplication::setStyle(styleName);

    if (!shotDir.isEmpty()) {
        QDir().mkpath(shotDir);
        for (const Section &s : kSections) {
            QWidget *w = s.make();
            w->setAttribute(Qt::WA_KeyboardFocusChange, true);
            w->show();
            app.processEvents();
            app.processEvents();
            const QString path = QString("%1/%2@%3x.png").arg(shotDir, s.name, scale);
            if (!w->grab().save(path)) qWarning("failed to write %s", qPrintable(path));
            delete w;
        }
        return 0;
    }

    // Interactive: all sections in a scrolling column.
    auto *all = new QWidget;
    auto *v = new QVBoxLayout(all);
    for (const Section &s : kSections) {
        if (QByteArray(s.name) == "menu" || QByteArray(s.name) == "tooltip") continue;
        v->addWidget(new QLabel(QString("<b>%1</b>").arg(s.name)));
        v->addWidget(s.make());
    }
    auto *sa = new QScrollArea; sa->setWidget(all); sa->setWidgetResizable(true); sa->resize(520, 800);
    sa->setWindowTitle("Adwaita gallery");
    sa->show();
    return app.exec();
}
