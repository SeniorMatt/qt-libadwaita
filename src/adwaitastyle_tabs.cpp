// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: tab bars, tab widgets, tool bars, tool buttons, group boxes, frames, splitters
#include "adwaitastyle.h"

bool AdwaitaStyle::tabs_drawPrimitive(PrimitiveElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::tabs_drawControl(ControlElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::tabs_drawComplexControl(ComplexControl, const QStyleOptionComplex *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::tabs_subElementRect(SubElement, const QStyleOption *, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::tabs_subControlRect(ComplexControl, const QStyleOptionComplex *, SubControl, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::tabs_pixelMetric(PixelMetric, const QStyleOption *, const QWidget *, int &) const { return false; }
bool AdwaitaStyle::tabs_sizeFromContents(ContentsType, const QStyleOption *, const QSize &, const QWidget *, QSize &) const { return false; }
bool AdwaitaStyle::tabs_styleHint(StyleHint, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &) const { return false; }
void AdwaitaStyle::tabs_polish(QWidget *) {}
void AdwaitaStyle::tabs_unpolish(QWidget *) {}
