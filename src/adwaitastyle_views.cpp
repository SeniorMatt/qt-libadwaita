// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: item views, list/tree/table, header views, item selection
#include "adwaitastyle.h"

bool AdwaitaStyle::views_drawPrimitive(PrimitiveElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::views_drawControl(ControlElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::views_drawComplexControl(ComplexControl, const QStyleOptionComplex *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::views_subElementRect(SubElement, const QStyleOption *, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::views_subControlRect(ComplexControl, const QStyleOptionComplex *, SubControl, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::views_pixelMetric(PixelMetric, const QStyleOption *, const QWidget *, int &) const { return false; }
bool AdwaitaStyle::views_sizeFromContents(ContentsType, const QStyleOption *, const QSize &, const QWidget *, QSize &) const { return false; }
bool AdwaitaStyle::views_styleHint(StyleHint, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &) const { return false; }
void AdwaitaStyle::views_polish(QWidget *) {}
void AdwaitaStyle::views_unpolish(QWidget *) {}
