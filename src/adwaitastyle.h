// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "adwaitatokens.h"
#include <QCommonStyle>

// App-facing API (dynamic properties, mirroring GTK style classes):
//   "adwaita-class" = suggested-action | destructive-action | flat | pill | headerbar | boxed-list | boxed-row
//   "adwaita-switch" = true on a QCheckBox draws a switch.
//   "adwaita-focus-visible" = true forces the focus ring (test hook).
class AdwaitaStyle : public QCommonStyle
{
    Q_OBJECT
public:
    AdwaitaStyle();

    QPalette standardPalette() const override;
    void polish(QPalette &pal) override;
    void polish(QWidget *w) override;
    void unpolish(QWidget *w) override;
    using QCommonStyle::polish;
    using QCommonStyle::unpolish;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const override;
    void drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const override;
    QRect subElementRect(SubElement se, const QStyleOption *opt, const QWidget *w) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const override;
    int pixelMetric(PixelMetric pm, const QStyleOption *opt, const QWidget *w) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &sz, const QWidget *w) const override;
    int styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *w, QStyleHintReturn *ret) const override;

    const AdwaitaTokens &tokens() const { return m_tokens; }

    // ---- helpers shared by the per-family files ----
    enum ButtonKind { Normal, Suggested, Destructive, Flat, Pill };
    static ButtonKind buttonKind(const QWidget *w, const QStyleOption *opt);
    static bool isSwitch(const QWidget *w);
    static QString adwClass(const QWidget *w);
    static bool focusVisible(const QStyleOption *opt, const QWidget *w);
    QColor withDisabled(const QColor &c, const QStyleOption *opt) const;
    void drawFocusRing(QPainter *p, const QRectF &r, qreal radius) const;  // ring drawn on r's outline
    QColor buttonFill(ButtonKind k, const QStyleOption *opt) const;

    // per widget family (adwaitastyle_*.cpp); return true if handled
    bool drawButtonBevel(const QStyleOption *opt, QPainter *p, const QWidget *w) const;
    bool drawButtonLabel(const QStyleOption *opt, QPainter *p, const QWidget *w) const;
    bool drawEntryPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const;
    bool drawComboBox(const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const;
    bool drawCheckPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const;

private slots:
    void settingsChanged();

private:
    void reload();
    AdwaitaTokens m_tokens;
};
