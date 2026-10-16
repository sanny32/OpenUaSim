// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appstyle.cpp
/// \brief Implements the application proxy style that draws panels, tabs and accent buttons.
///

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QStyleFactory>
#include <QStyleOptionTab>
#include <QWidget>

#include "appcolors.h"
#include "appstyle.h"

namespace {

// Heights are fixed to the design so the proportions hold with any system font.
constexpr int kTabGap = 2;
constexpr int kTabPadding = 22;
constexpr int kTabHeight = 34;
constexpr int kToolbarExtraWidth = 38;
constexpr int kToolbarHeight = 34;
constexpr int kInputHeight = 30;
constexpr int kHeaderHeight = 29;
constexpr int kHeaderMargin = 6;
constexpr int kItemTextMargin = 10;
constexpr int kItemIconSize = 18;
constexpr int kTreeIndentation = 22;
constexpr int kCheckIndicatorSize = 14;

///
/// \brief Reports whether a widget sits in the main toolbar.
/// \param widget Widget being measured.
/// \return True when the widget carries the toolbar property.
///
bool isToolbarWidget(const QWidget *widget)
{
    return widget && widget->property(AppStyle::toolbarProperty).toBool();
}

///
/// \brief Returns the accent colour a button asks for.
/// \param widget Button being drawn.
/// \return Accent colour, or an invalid colour when the button has no known accent.
///
QColor accentFor(const QWidget *widget)
{
    if (!widget)
        return {};
    const QString accent = widget->property(AppStyle::accentProperty).toString();
    if (accent == QLatin1String("success"))
        return AppColors::status(StatusKind::Good);
    return {};
}

///
/// \brief Returns a colour with a different alpha.
/// \param color Base colour.
/// \param alpha New alpha, 0-255.
/// \return Translucent colour.
///
QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

}

///
/// \brief Constructs the style on top of Fusion.
///
AppStyle::AppStyle()
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
{
}

///
/// \brief Draws panel frames, separators, check boxes and accent button bevels; defers
///        everything else to Fusion.
/// \param element Primitive element to draw.
/// \param option Style option of the element.
/// \param painter Painter to draw with.
/// \param widget Widget being drawn.
///
void AppStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                             const QWidget *widget) const
{
    if (element == PE_Frame && widget && widget->property(panelProperty).toBool()) {
        drawPanel(option, painter);
        return;
    }
    if (element == PE_Frame && widget && widget->property(separatorProperty).toBool()) {
        painter->fillRect(option->rect, option->palette.color(QPalette::Mid));
        return;
    }
    if (element == PE_IndicatorCheckBox || element == PE_IndicatorItemViewItemCheck) {
        drawCheckIndicator(option, painter);
        return;
    }
    if (element == PE_PanelButtonCommand && option->state.testFlag(State_Enabled)) {
        const QColor accent = accentFor(widget);
        if (accent.isValid()) {
            drawAccentButton(option, painter, accent);
            return;
        }
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

///
/// \brief Draws tab shapes and labels; defers everything else to Fusion.
/// \param element Control element to draw.
/// \param option Style option of the element.
/// \param painter Painter to draw with.
/// \param widget Widget being drawn.
///
void AppStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                           const QWidget *widget) const
{
    if (element == CE_TabBarTabShape) {
        drawTabShape(option, painter);
        return;
    }
    if (element == CE_TabBarTabLabel) {
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            QStyleOptionTab label(*tab);
            label.rect.adjust(0, 0, -kTabGap, 0);
            if (tab->state.testFlag(State_Selected)) {
                const QColor text = tab->palette.color(QPalette::HighlightedText);
                label.palette.setColor(QPalette::WindowText, text);
                label.palette.setColor(QPalette::ButtonText, text);
            }
            QProxyStyle::drawControl(element, &label, painter, widget);
            return;
        }
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

///
/// \brief Raises tabs, input fields, header sections and toolbar controls to the design heights.
/// \param type Contents type being measured.
/// \param option Style option of the control.
/// \param contentsSize Size requested by the contents.
/// \param widget Widget being measured.
/// \return Adjusted size.
///
QSize AppStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                 const QSize &contentsSize, const QWidget *widget) const
{
    QSize size = QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
    switch (type) {
    case CT_TabBarTab:
        size.rwidth() += kTabGap + kTabPadding;
        size.setHeight(qMax(size.height(), kTabHeight));
        break;
    case CT_PushButton:
        if (isToolbarWidget(widget)) {
            size.rwidth() += kToolbarExtraWidth;
            size.setHeight(qMax(size.height(), kToolbarHeight));
        }
        break;
    case CT_HeaderSection:
        size.setHeight(qMax(size.height(), kHeaderHeight));
        break;
    case CT_LineEdit:
        // The editor inside a spin box or combo box grows with its owner instead.
        if (widget && (qobject_cast<const QAbstractSpinBox *>(widget->parentWidget())
                       || qobject_cast<const QComboBox *>(widget->parentWidget())))
            break;
        Q_FALLTHROUGH();
    case CT_ComboBox:
    case CT_SpinBox:
        size.setHeight(qMax(size.height(), isToolbarWidget(widget) ? kToolbarHeight : kInputHeight));
        break;
    default:
        break;
    }
    return size;
}

///
/// \brief Sets item view text margins, icon size and tree indentation, header margins and
///        check box indicator size.
/// \param metric Metric to query.
/// \param option Style option of the control.
/// \param widget Widget being measured.
/// \return Metric value in pixels.
///
int AppStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    // Applies on all sides, so it also sets the header height and the minimum row height.
    if (metric == PM_HeaderMargin)
        return kHeaderMargin;
    if (metric == PM_TreeViewIndentation)
        return kTreeIndentation;
    if (metric == PM_IndicatorWidth || metric == PM_IndicatorHeight)
        return kCheckIndicatorSize;
    if (qobject_cast<const QAbstractItemView *>(widget)) {
        // Fusion pads item view text by this margin plus one.
        if (metric == PM_FocusFrameHMargin)
            return kItemTextMargin - 1;
        if (metric == PM_SmallIconSize)
            return kItemIconSize;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

///
/// \brief Makes panel scroll areas transparent so the panel colour shows through.
/// \param widget Widget being polished.
///
void AppStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    auto *scroll = qobject_cast<QScrollArea *>(widget);
    if (!scroll || !scroll->property(panelBodyProperty).toBool())
        return;
    scroll->viewport()->setAutoFillBackground(false);
    if (scroll->widget())
        scroll->widget()->setAutoFillBackground(false);
}

///
/// \brief Draws a check box as a rounded square: accent-filled with a white tick when checked,
///        outlined when unchecked.
/// \param option Style option carrying the state, rectangle and palette.
/// \param painter Painter to draw with.
///
void AppStyle::drawCheckIndicator(const QStyleOption *option, QPainter *painter) const
{
    const QPalette &palette = option->palette;
    const bool enabled = option->state.testFlag(State_Enabled);
    const bool checked = option->state.testFlag(State_On);
    const bool partial = option->state.testFlag(State_NoChange);
    const QRectF box = QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    if (checked || partial) {
        const QColor fill = enabled ? palette.color(QPalette::Highlight)
                                    : palette.color(QPalette::Disabled, QPalette::Highlight);
        painter->setPen(fill);
        painter->setBrush(fill);
        painter->drawRoundedRect(box, 3, 3);

        QPen mark(palette.color(QPalette::HighlightedText), qMax(1.5, box.width() / 8.0),
                  Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(mark);
        painter->setBrush(Qt::NoBrush);
        const qreal w = box.width();
        const qreal h = box.height();
        const QPointF origin = box.topLeft();
        if (partial) {
            painter->drawLine(origin + QPointF(w * 0.25, h * 0.5), origin + QPointF(w * 0.75, h * 0.5));
        } else {
            const QPointF tick[] = { origin + QPointF(w * 0.24, h * 0.52),
                                     origin + QPointF(w * 0.43, h * 0.70),
                                     origin + QPointF(w * 0.77, h * 0.32) };
            painter->drawPolyline(tick, 3);
        }
    } else {
        painter->setPen(palette.color(enabled ? QPalette::Active : QPalette::Disabled, QPalette::Text)
                            .darker(enabled ? 140 : 100));
        painter->setBrush(palette.color(QPalette::Base));
        painter->drawRoundedRect(box, 3, 3);
    }
    painter->restore();
}

///
/// \brief Draws a rounded box filled with the base colour and outlined with the mid colour.
/// \param option Style option carrying the rectangle and palette.
/// \param painter Painter to draw with.
///
void AppStyle::drawPanel(const QStyleOption *option, QPainter *painter) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(option->palette.color(QPalette::Mid));
    painter->setBrush(option->palette.color(QPalette::Base));
    painter->drawRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                             cornerRadius(), cornerRadius());
    painter->restore();
}

///
/// \brief Draws a push button tinted and outlined in an accent colour.
/// \param option Style option of the button.
/// \param painter Painter to draw with.
/// \param accent Accent colour.
///
void AppStyle::drawAccentButton(const QStyleOption *option, QPainter *painter, const QColor &accent) const
{
    const bool sunken = option->state.testFlag(State_Sunken) || option->state.testFlag(State_On);
    const bool hover = option->state.testFlag(State_MouseOver);
    const int alpha = sunken ? 90 : (hover ? 60 : 36);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(accent);
    painter->setBrush(withAlpha(accent, alpha));
    painter->drawRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
    painter->restore();
}

///
/// \brief Draws a tab as a separate rounded box; the selected tab is filled with the accent.
/// \param option Tab style option.
/// \param painter Painter to draw with.
///
void AppStyle::drawTabShape(const QStyleOption *option, QPainter *painter) const
{
    const QPalette &palette = option->palette;
    const bool selected = option->state.testFlag(State_Selected);
    const bool hover = option->state.testFlag(State_MouseOver);

    QColor fill = palette.color(QPalette::Button);
    if (selected)
        fill = palette.color(QPalette::Highlight);
    else if (hover)
        fill = palette.color(QPalette::Midlight);

    const QRectF box = QRectF(option->rect).adjusted(0.5, 0.5, -kTabGap - 0.5, 0.5);
    QPainterPath path;
    path.addRoundedRect(box, cornerRadius() - 2, cornerRadius() - 2);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(selected ? fill : palette.color(QPalette::Mid));
    painter->setBrush(fill);
    painter->drawPath(path);
    painter->restore();
}
