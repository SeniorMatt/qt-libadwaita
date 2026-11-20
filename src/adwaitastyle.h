// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "adwaitatokens.h"
#include <QCommonStyle>
#include <QImage>
#include <QPainter>

// App-facing API (dynamic properties, mirroring GTK style classes):
//   "adwaita-class" = suggested-action | destructive-action | flat | pill | headerbar | boxed-list | boxed-row
//   "adwaita-switch" = true on a QCheckBox draws a switch.
//   "adwaita-focus-visible" = true forces the focus ring (test hook).

// Family hooks: each adwaitastyle_<family>.cpp implements these; return true when handled (result in out).
#define ADW_FAMILY_HOOKS(F) \
    bool F##_drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const; \
    bool F##_drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const; \
    bool F##_drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const; \
    bool F##_subElementRect(SubElement se, const QStyleOption *opt, const QWidget *w, QRect &out) const; \
    bool F##_subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *w, QRect &out) const; \
    bool F##_pixelMetric(PixelMetric pm, const QStyleOption *opt, const QWidget *w, int &out) const; \
    bool F##_sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &sz, const QWidget *w, QSize &out) const; \
    bool F##_styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *w, QStyleHintReturn *ret, int &out) const; \
    void F##_polish(QWidget *w); \
    void F##_unpolish(QWidget *w);
#define ADW_FAMILIES(X) X(scroll) X(range) X(menus) X(tabs) X(views)

qreal adwSpacingScale();  // user spacing option (1 = libadwaita)

class AdwaitaStyle : public QCommonStyle
{
    Q_OBJECT
public:
    AdwaitaStyle();

    QPalette standardPalette() const override;
    void polish(QPalette &pal) override;
    void polish(QWidget *w) override;
    void unpolish(QWidget *w) override;
    void polish(QApplication *app) override;
    void unpolish(QApplication *app) override;
    bool eventFilter(QObject *o, QEvent *e) override;
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
    bool drawComboLabel(const QStyleOption *opt, QPainter *p, const QWidget *w) const;
    static bool hasClass(const QWidget *w, const char *cls);
    static bool isPillButton(const QWidget *w);
    static bool focusVisible(const QStyleOption *opt, const QWidget *w);
    QColor withDisabled(const QColor &c, const QStyleOption *opt) const;
    void drawFocusRing(QPainter *p, const QRectF &r, qreal radius) const;  // ring drawn on r's outline
    QColor buttonFill(ButtonKind k, const QStyleOption *opt) const;

    // per widget family (adwaitastyle_*.cpp); return true if handled
    bool drawButtonBevel(const QStyleOption *opt, QPainter *p, const QWidget *w) const;
    bool drawButtonLabel(const QStyleOption *opt, QPainter *p, const QWidget *w) const;
    bool drawEntryPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const;
    bool drawComboBox(const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const;
    // libadwaita fades a disabled widget as one group (opacity on the whole widget): paint into a layer, blend once.
    template<class F> void drawFaded(QPainter *p, const QRect &r, F &&paint) const
    {
        const qreal dpr = p->device()->devicePixelRatioF();
        const QRect lr = r.adjusted(-6, -6, 6, 6);
        QImage img((lr.size() * dpr).expandedTo(QSize(1, 1)), QImage::Format_ARGB32_Premultiplied);
        img.setDevicePixelRatio(dpr);
        img.fill(Qt::transparent);
        QPainter ip(&img);
        ip.translate(-lr.topLeft());
        paint(&ip);
        ip.end();
        p->save();
        p->setOpacity(p->opacity() * m_tokens.disabled_opacity);
        p->drawImage(lr.topLeft(), img);
        p->restore();
    }
    void polishSwitch(QWidget *w);
    bool drawCheckPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const;

    ADW_FAMILIES(ADW_FAMILY_HOOKS)

private slots:
    void settingsChanged();

private:
    void reload();
    AdwaitaTokens m_tokens;
};
