// SPDX-License-Identifier: GPL-2.0-or-later
// Widget family: scroll bars (overlay), scroll areas
#include "adwaitastyle.h"

bool AdwaitaStyle::scroll_drawPrimitive(PrimitiveElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::scroll_drawControl(ControlElement, const QStyleOption *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::scroll_drawComplexControl(ComplexControl, const QStyleOptionComplex *, QPainter *, const QWidget *) const { return false; }
bool AdwaitaStyle::scroll_subElementRect(SubElement, const QStyleOption *, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::scroll_subControlRect(ComplexControl, const QStyleOptionComplex *, SubControl, const QWidget *, QRect &) const { return false; }
bool AdwaitaStyle::scroll_pixelMetric(PixelMetric, const QStyleOption *, const QWidget *, int &) const { return false; }
bool AdwaitaStyle::scroll_sizeFromContents(ContentsType, const QStyleOption *, const QSize &, const QWidget *, QSize &) const { return false; }
bool AdwaitaStyle::scroll_styleHint(StyleHint, const QStyleOption *, const QWidget *, QStyleHintReturn *, int &) const { return false; }
void AdwaitaStyle::scroll_polish(QWidget *) {}
void AdwaitaStyle::scroll_unpolish(QWidget *) {}
