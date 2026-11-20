// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: menus, menu bars, combo popups, tooltips
#include "adwaitastyle.h"

bool AdwaitaStyle::menus_drawPrimitive(PrimitiveElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::menus_drawControl(ControlElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::menus_drawComplexControl(ComplexControl, const QStyleOptionComplex *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::menus_subElementRect(SubElement, const QStyleOption *, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::menus_subControlRect(ComplexControl, const QStyleOptionComplex *, SubControl, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::menus_pixelMetric(PixelMetric, const QStyleOption *, const QWidget *, int &) const { return false; }
bool AdwaitaStyle::menus_sizeFromContents(ContentsType, const QStyleOption *, const QSize &, const QWidget *, QSize &) const { return false; }
bool AdwaitaStyle::menus_styleHint(StyleHint, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &) const { return false; }
void AdwaitaStyle::menus_polish(QWidget *) {}
void AdwaitaStyle::menus_unpolish(QWidget *) {}
