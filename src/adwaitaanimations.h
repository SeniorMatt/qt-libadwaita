// SPDX-License-Identifier: GPL-2.0-or-later
// Small shared animation engine for the widget style (libadwaita CSS transitions).
//
// Paint code asks for the displayed value of a (widget, key) channel, passing the value the widget's current state
// calls for. The engine remembers the previous target; when it changes it starts a timed transition from the value
// displayed right now and returns the in-between value on later calls. One shared 16 ms timer repaints the animated
// widgets and stops by itself when nothing moves, so an idle UI has no timers. State is dropped when the widget is
// destroyed. A channel seen for the first time (or while animations are off / the widget is hidden) just returns its
// target: static end states are exactly what the paint code computes without the engine.
// No Q_OBJECT classes: nothing here needs moc.
#pragma once
#include <QColor>
#include <QtGlobal>

class QWidget;

namespace AdwAnim {

enum Curve { Button, EaseInOut, EaseOut };  // cubic-bezier(.25,.46,.45,.94), ease-in-out, ease-out

// channel ids (high half of the key); idx (low half) separates sub-controls / model indexes
enum Channel : quint32 { Fill = 1, Fill2, Fill3, Ring, Row, Branch, Expand, Knob, Trough };
constexpr quint64 key(Channel c, quint32 idx = 0) { return (quint64(c) << 32) | idx; }

QColor color(const QWidget *w, quint64 key, const QColor &target, Curve c = Button, int ms = 200);
qreal value(const QWidget *w, quint64 key, qreal target, Curve c = Button, int ms = 200);
// Start a transition by hand (signal driven, e.g. a tree item expanding).
void kick(const QWidget *w, quint64 key, qreal from, qreal to, Curve c = Button, int ms = 200);
// Value of a running transition, else `rest` (does not create state).
qreal running(const QWidget *w, quint64 key, qreal rest);

QColor mix(const QColor &a, const QColor &b, qreal t);  // straight-alpha colours blended in premultiplied space

}  // namespace AdwAnim
