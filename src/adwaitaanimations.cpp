// SPDX-License-Identifier: GPL-2.0-or-later
#include "adwaitaanimations.h"
#include "adwaitasettings.h"

#include <QAbstractScrollArea>
#include <QElapsedTimer>
#include <QEasingCurve>
#include <QHash>
#include <QCoreApplication>
#include <QSet>
#include <QTimer>
#include <QWidget>

namespace AdwAnim {
namespace {

struct Entry {
    QColor cf, ct;
    qreal sf = 0, st = 0;
    qint64 t0 = 0;
    int ms = 200;
    Curve curve = Button;
    bool anim = false;
};
using Tracker = QHash<quint64, Entry>;

constexpr int kMaxEntries = 4096;  // per widget (item views key one entry per painted cell)

QHash<const QObject *, Tracker> g_trackers;
QSet<const QObject *> g_active;
QObject *g_owner = nullptr;
QTimer *g_timer = nullptr;
QElapsedTimer g_clock;

qint64 now()
{
    if (!g_clock.isValid()) g_clock.start();
    return g_clock.elapsed();
}

qreal ease(Curve c, qreal x)
{
    static const QEasingCurve curves[3] = {
        [] { QEasingCurve e(QEasingCurve::BezierSpline); e.addCubicBezierSegment(QPointF(.25, .46), QPointF(.45, .94), QPointF(1, 1)); return e; }(),
        [] { QEasingCurve e(QEasingCurve::BezierSpline); e.addCubicBezierSegment(QPointF(.42, 0), QPointF(.58, 1), QPointF(1, 1)); return e; }(),
        [] { QEasingCurve e(QEasingCurve::BezierSpline); e.addCubicBezierSegment(QPointF(0, 0), QPointF(.58, 1), QPointF(1, 1)); return e; }(),
    };
    return curves[c].valueForProgress(x);
}

void dropRest(Tracker &tr)  // cap reached: forget entries at rest (they are re-learnt on the next paint)
{
    for (auto i = tr.begin(); i != tr.end();) {
        if (i->anim) ++i;
        else i = tr.erase(i);
    }
}

// 0..1 eased progress of a running entry; 1 when finished
qreal progressOf(const Entry &e, qint64 t)
{
    if (!e.anim) return 1;
    const qreal x = qreal(t - e.t0) / qMax(1, e.ms);
    return x >= 1 ? 1 : ease(e.curve, qMax<qreal>(0, x));
}

void repaint(const QObject *o)
{
    auto *w = static_cast<QWidget *>(const_cast<QObject *>(o));
    if (!w->isVisible()) return;
    if (auto *sa = qobject_cast<QAbstractScrollArea *>(w)) sa->viewport()->update();
    else w->update();
}

void tick()
{
    const qint64 t = now();
    const QList<const QObject *> act = g_active.values();
    for (const QObject *o : act) {
        auto it = g_trackers.find(o);
        if (it == g_trackers.end()) { g_active.remove(o); continue; }
        bool still = false;
        for (Entry &e : it.value()) {
            if (!e.anim) continue;
            if (t - e.t0 >= e.ms) e.anim = false;  // finished: the next paint sees the end value
            else still = true;
        }
        repaint(o);
        if (!still) g_active.remove(o);
    }
    if (g_active.isEmpty() && g_timer) g_timer->stop();
}

void activate(const QObject *o)
{
    g_active.insert(o);
    if (!g_timer) {
        g_timer = new QTimer(g_owner);
        g_timer->setInterval(16);
        g_timer->setTimerType(Qt::PreciseTimer);
        QObject::connect(g_timer, &QTimer::timeout, g_owner, [] { tick(); });
    }
    if (!g_timer->isActive()) g_timer->start();
}

Tracker &trackerFor(const QWidget *w)
{
    auto it = g_trackers.find(w);
    if (it != g_trackers.end()) return it.value();
    if (!g_owner) g_owner = new QObject(qApp);
    QObject::connect(w, &QObject::destroyed, g_owner, [](QObject *o) { g_trackers.remove(o); g_active.remove(o); });
    return g_trackers[w];
}

bool canAnimate(const QWidget *w) { return w && w->isVisible() && AdwaitaSettings::instance()->animationsEnabled(); }

}  // namespace

QColor mix(const QColor &a, const QColor &b, qreal t)
{
    if (t <= 0) return a;
    if (t >= 1) return b;
    const qreal aa = a.alphaF(), ab = b.alphaF();
    const qreal al = aa + (ab - aa) * t;
    if (al <= 0) return QColor::fromRgbF(b.redF(), b.greenF(), b.blueF(), 0);
    auto ch = [&](qreal ca, qreal cb) { return (ca * aa + (cb * ab - ca * aa) * t) / al; };
    return QColor::fromRgbF(ch(a.redF(), b.redF()), ch(a.greenF(), b.greenF()), ch(a.blueF(), b.blueF()), al);
}

QColor color(const QWidget *w, quint64 k, const QColor &target, Curve c, int ms)
{
    if (!w) return target;
    Tracker &tr = trackerFor(w);
    auto it = tr.find(k);
    if (it == tr.end()) {
        if (tr.size() >= kMaxEntries) {
            dropRest(tr);
        }
        Entry e; e.cf = e.ct = target;
        tr.insert(k, e);
        return target;
    }
    Entry &e = it.value();
    const qint64 t = now();
    if (e.ct != target) {
        const QColor shown = e.anim ? mix(e.cf, e.ct, progressOf(e, t)) : e.ct;
        e.cf = shown; e.ct = target;
        if (!canAnimate(w)) { e.cf = target; e.anim = false; return target; }
        e.t0 = t; e.ms = ms; e.curve = c; e.anim = true;
        activate(w);
        return shown;
    }
    return e.anim ? mix(e.cf, e.ct, progressOf(e, t)) : e.ct;
}

qreal value(const QWidget *w, quint64 k, qreal target, Curve c, int ms)
{
    if (!w) return target;
    Tracker &tr = trackerFor(w);
    auto it = tr.find(k);
    if (it == tr.end()) {
        Entry e; e.sf = e.st = target;
        tr.insert(k, e);
        return target;
    }
    Entry &e = it.value();
    const qint64 t = now();
    const qreal shown = e.anim ? e.sf + (e.st - e.sf) * progressOf(e, t) : e.st;
    if (!qFuzzyCompare(e.st + 1, target + 1)) {
        e.sf = shown; e.st = target;
        if (!canAnimate(w)) { e.sf = target; e.anim = false; return target; }
        e.t0 = t; e.ms = ms; e.curve = c; e.anim = true;
        activate(w);
    }
    return shown;
}

void kick(const QWidget *w, quint64 k, qreal from, qreal to, Curve c, int ms)
{
    if (!w || !canAnimate(w)) return;
    Tracker &tr = trackerFor(w);
    Entry e;
    e.sf = from; e.st = to; e.t0 = now(); e.ms = ms; e.curve = c; e.anim = true;
    tr.insert(k, e);
    activate(w);
}

qreal running(const QWidget *w, quint64 k, qreal rest)
{
    if (!w) return rest;
    auto tit = g_trackers.constFind(w);
    if (tit == g_trackers.constEnd()) return rest;
    auto it = tit->constFind(k);
    if (it == tit->constEnd() || !it->anim) return rest;
    const qint64 t = now();
    if (t - it->t0 >= it->ms) return rest;
    return it->sf + (it->st - it->sf) * progressOf(*it, t);
}

}  // namespace AdwAnim
