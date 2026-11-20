// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: sliders, progress bars, spin boxes
#include "adwaitastyle.h"

bool AdwaitaStyle::range_drawPrimitive(PrimitiveElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::range_drawControl(ControlElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::range_drawComplexControl(ComplexControl, const QStyleOptionComplex *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::range_subElementRect(SubElement, const QStyleOption *, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::range_subControlRect(ComplexControl, const QStyleOptionComplex *, SubControl, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::range_pixelMetric(PixelMetric, const QStyleOption *, const QWidget *, int &) const { return false; }
bool AdwaitaStyle::range_sizeFromContents(ContentsType, const QStyleOption *, const QSize &, const QWidget *, QSize &) const { return false; }
bool AdwaitaStyle::range_styleHint(StyleHint, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &) const { return false; }
void AdwaitaStyle::range_polish(QWidget *) {}
void AdwaitaStyle::range_unpolish(QWidget *) {}
