// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appstyle.cpp
/// \brief Implements the application proxy style that draws panels, tabs and accent buttons.
///

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QStyleFactory>
#include <QStyleOptionButton>
#include <QStyleOptionToolButton>
#include <QStyleOptionTab>
#include <QStyleOptionViewItem>
#include <QWidget>

#include "appcolors.h"
#include "appicons.h"
#include "appstyle.h"

namespace {

// Heights are fixed to the design so the proportions hold with any system font.
constexpr int kTabGap = 2;
constexpr int kTabPadding = 22;
constexpr int kTabHeight = 34;
constexpr int kToolbarExtraWidth = 38;
constexpr int kToolbarHeight = 34;
constexpr int kFlatToolbarPadding = 10;
constexpr int kToolbarIconGap = 10;
// QPushButton::sizeHint() reserves this much between the icon and the text.
constexpr int kPushButtonIconSpacing = 4;
constexpr int kInputHeight = 30;
constexpr int kHeaderHeight = 29;
constexpr int kHeaderMargin = 6;
constexpr int kItemTextMargin = 10;
constexpr int kItemIconSize = 18;
constexpr int kItemIconTextGap = 6;
constexpr int kTreeIndentation = 22;
constexpr int kCheckIndicatorSize = 14;
constexpr int kMessageBoxIconSize = 32;
constexpr int kMessageBoxMinimumWidth = 360;
constexpr const char *kMessageBoxWidenedProperty = "appStyleWidened";

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
/// \brief Sets the main message box text in semibold and the informative text in the secondary colour.
/// \param widget Widget being polished; anything but a message box text label is left alone.
///
void polishMessageBoxText(QWidget *widget)
{
    // Object names given by QMessageBox to its text labels.
    if (!qobject_cast<QLabel *>(widget) || !qobject_cast<QMessageBox *>(widget->parentWidget()))
        return;
    if (widget->objectName() == QLatin1String("qt_msgbox_label")) {
        QFont font = widget->font();
        font.setWeight(QFont::DemiBold);
        widget->setFont(font);
    } else if (widget->objectName() == QLatin1String("qt_msgbox_informativelabel")) {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::WindowText, AppColors::secondaryText(QGuiApplication::palette()));
        widget->setPalette(palette);
    }
}

///
/// \brief Returns the button option of a toolbar button showing both an icon and a text.
/// \param option Style option of the control.
/// \param widget Widget being drawn or measured.
/// \return Button option, or null for any other control.
///
const QStyleOptionButton *labelledToolbarButton(const QStyleOption *option, const QWidget *widget)
{
    const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option);
    if (!button || !isToolbarWidget(widget) || button->icon.isNull() || button->text.isEmpty())
        return nullptr;
    return button;
}

///
/// \brief Returns the width of the icon, the design gap and the text of a toolbar button.
/// \param button Button option.
/// \return Label width in pixels.
///
int toolbarLabelWidth(const QStyleOptionButton *button)
{
    return button->iconSize.width() + kToolbarIconGap
           + button->fontMetrics.size(Qt::TextShowMnemonic, button->text).width();
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
/// \brief Reports whether a button is the filled main action of a dialog.
/// \param widget Button being drawn.
/// \return True when the button's accent is "primary".
///
bool isPrimaryButton(const QWidget *widget)
{
    return widget && widget->property(AppStyle::accentProperty).toString() == QLatin1String("primary");
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
        if (isPrimaryButton(widget)) {
            drawPrimaryButton(option, painter);
            return;
        }
        const QColor accent = accentFor(widget);
        if (accent.isValid()) {
            drawAccentButton(option, painter, accent);
            return;
        }
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

///
/// \brief Draws tab shapes and labels and toolbar and primary button labels; defers everything else to Fusion.
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
    if (element == CE_PushButton) {
        if (const QStyleOptionButton *button = labelledToolbarButton(option, widget)) {
            proxy()->drawControl(CE_PushButtonBevel, button, painter, widget);
            drawToolbarButtonLabel(button, painter);
            if (button->state.testFlag(State_HasFocus)) {
                QStyleOptionFocusRect focus;
                focus.QStyleOption::operator=(*button);
                focus.rect = subElementRect(SE_PushButtonFocusRect, button, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &focus, painter, widget);
            }
            return;
        }
    }
    if (element == CE_PushButtonLabel && option->state.testFlag(State_Enabled) && isPrimaryButton(widget)) {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            QStyleOptionButton label(*button);
            label.palette.setColor(QPalette::ButtonText, button->palette.color(QPalette::HighlightedText));
            QProxyStyle::drawControl(element, &label, painter, widget);
            return;
        }
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
/// \brief Draws toolbar menu buttons with a separate chevron and rounded hover feedback.
/// \param control Complex control to draw.
/// \param option Style option of the control.
/// \param painter Painter to draw with.
/// \param widget Widget being drawn.
///
void AppStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                  QPainter *painter, const QWidget *widget) const
{
    const auto *button = qstyleoption_cast<const QStyleOptionToolButton *>(option);
    if (control != CC_ToolButton || !button || !isToolbarWidget(widget)
        || !button->features.testFlag(QStyleOptionToolButton::HasMenu)) {
        QProxyStyle::drawComplexControl(control, option, painter, widget);
        return;
    }

    const bool enabled = button->state.testFlag(State_Enabled);
    const bool pressed = button->state.testFlag(State_Sunken);
    const bool highlighted = enabled && (pressed || button->state.testFlag(State_MouseOver)
                                        || button->state.testFlag(State_HasFocus));
    const QPalette::ColorGroup group = enabled ? QPalette::Active : QPalette::Disabled;
    const QColor foreground = button->palette.color(group, QPalette::ButtonText);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(highlighted ? withAlpha(button->palette.color(QPalette::Highlight), 150)
                               : withAlpha(foreground, 35));
    painter->setBrush(highlighted ? withAlpha(button->palette.color(QPalette::Highlight), pressed ? 45 : 25)
                                 : withAlpha(foreground, 8));
    painter->drawRoundedRect(QRectF(button->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                             cornerRadius(), cornerRadius());

    const QRect iconRect = visualRect(button->direction, button->rect,
                                     QRect(button->rect.left() + 9,
                                           button->rect.center().y() - button->iconSize.height() / 2 + 1,
                                           button->iconSize.width(), button->iconSize.height()));
    button->icon.paint(painter, iconRect, Qt::AlignCenter,
                       enabled ? QIcon::Normal : QIcon::Disabled);
    const QRect arrowRect = visualRect(button->direction, button->rect,
                                      QRect(button->rect.right() - 15, button->rect.center().y() - 1, 6, 3));
    painter->setPen(QPen(foreground, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(QPolygonF{QPointF(arrowRect.left(), arrowRect.top()),
                                    QPointF(arrowRect.left() + 3, arrowRect.bottom() + 1),
                                    QPointF(arrowRect.right() + 1, arrowRect.top())});
    painter->restore();
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
    case CT_ToolButton:
        if (isToolbarWidget(widget))
            size = QSize(qMax(size.width(), contentsSize.width() + 32),
                         qMax(size.height(), kToolbarHeight));
        break;
    case CT_PushButton:
        if (!isToolbarWidget(widget))
            break;
        size.setHeight(qMax(size.height(), kToolbarHeight));
        if (const QStyleOptionButton *button = labelledToolbarButton(option, widget)) {
            if (button->features.testFlag(QStyleOptionButton::Flat)) {
                size.setWidth(2 * kFlatToolbarPadding + toolbarLabelWidth(button));
                break;
            }
            size.rwidth() += kToolbarIconGap - kPushButtonIconSpacing;
        }
        size.rwidth() += kToolbarExtraWidth;
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
///        check box indicator and message box icon sizes.
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
    if (metric == PM_MessageBoxIconSize)
        return kMessageBoxIconSize;
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
/// \brief Pulls item view text towards a decoration on its left.
/// \param element Sub-element to place.
/// \param option Style option of the control.
/// \param widget Widget being laid out.
/// \return Sub-element rectangle.
///
QRect AppStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    QRect rect = QProxyStyle::subElementRect(element, option, widget);
    const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option);
    if (element != SE_ItemViewItemText || !item
        || !item->features.testFlag(QStyleOptionViewItem::HasDecoration)
        || item->decorationPosition != QStyleOptionViewItem::Left)
        return rect;
    // Fusion pads both the decoration and the text by the item margin, doubling the gap.
    const QRect decoration = QProxyStyle::subElementRect(SE_ItemViewItemDecoration, option, widget);
    const int textPadding = pixelMetric(PM_FocusFrameHMargin, option, widget) + 1;
    if (item->direction == Qt::RightToLeft)
        rect.setRight(decoration.left() - 1 - kItemIconTextGap + textPadding);
    else
        rect.setLeft(decoration.right() + 1 + kItemIconTextGap - textPadding);
    return rect;
}

///
/// \brief Replaces the platform message box icons with status-coloured badges.
/// \param standardIcon Icon to return.
/// \param option Style option of the requesting control.
/// \param widget Requesting widget.
/// \return Application icon for message boxes, the Fusion icon otherwise.
///
QIcon AppStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const
{
    const QIcon icon = AppIcons::messageIcon(standardIcon);
    return icon.isNull() ? QProxyStyle::standardIcon(standardIcon, option, widget) : icon;
}

///
/// \brief Adds vertical space, a minimum width and a text hierarchy to message boxes and makes panel
///        scroll areas transparent.
/// \param widget Widget being polished.
///
void AppStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    if (auto *box = qobject_cast<QMessageBox *>(widget)) {
        if (auto *layout = qobject_cast<QGridLayout *>(box->layout())) {
            QMargins margins = layout->contentsMargins();
            margins.setTop(20);
            margins.setBottom(20);
            layout->setContentsMargins(margins);
            layout->setVerticalSpacing(14);
            // QMessageBox sizes itself to its layout's minimum width, which otherwise follows the shortest text.
            if (!box->property(kMessageBoxWidenedProperty).toBool()) {
                box->setProperty(kMessageBoxWidenedProperty, true);
                layout->addItem(new QSpacerItem(kMessageBoxMinimumWidth, 0, QSizePolicy::Fixed, QSizePolicy::Minimum),
                                layout->rowCount(), 0, 1, layout->columnCount());
            }
        }
    }
    polishMessageBoxText(widget);
    auto *scroll = qobject_cast<QScrollArea *>(widget);
    if (!scroll || !scroll->property(panelBodyProperty).toBool())
        return;
    scroll->viewport()->setAutoFillBackground(false);
    if (scroll->widget())
        scroll->widget()->setAutoFillBackground(false);
}

///
/// \brief Starts watching the keyboard so menu mnemonics appear only after Alt, as on Windows.
/// \param application Application the style is set on.
///
void AppStyle::polish(QApplication *application)
{
    QProxyStyle::polish(application);
    application->installEventFilter(this);
}

///
/// \brief Stops watching the keyboard.
/// \param application Application the style is removed from.
///
void AppStyle::unpolish(QApplication *application)
{
    application->removeEventFilter(this);
    QProxyStyle::unpolish(application);
}

///
/// \brief Underlines mnemonics only while keyboard cues are on; defers everything else to Fusion.
/// \param hint Style hint to query.
/// \param option Style option of the control.
/// \param widget Widget being queried.
/// \param returnData Extra data the hint may return.
/// \return Hint value.
///
int AppStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                        QStyleHintReturn *returnData) const
{
    if (hint == SH_UnderlineShortcut)
        return _keyboardCues ? 1 : 0;
    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

///
/// \brief Turns keyboard cues on when Alt is pressed and off on a mouse press.
/// \param watched Object receiving the event.
/// \param event Event being delivered.
/// \return False, so the event is always delivered.
///
bool AppStyle::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Alt)
        setKeyboardCues(true);
    else if (event->type() == QEvent::MouseButtonPress)
        setKeyboardCues(false);
    return QProxyStyle::eventFilter(watched, event);
}

///
/// \brief Switches mnemonic underlines and repaints the menus that show them.
/// \param on Whether mnemonics are underlined.
///
void AppStyle::setKeyboardCues(bool on)
{
    if (_keyboardCues == on)
        return;
    _keyboardCues = on;
    const QWidgetList widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets) {
        if (qobject_cast<QMenuBar *>(widget) || qobject_cast<QMenu *>(widget))
            widget->update();
    }
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
/// \brief Draws a push button filled with the palette accent, with the gradient, border and top highlight
///        that give Fusion buttons their depth; darker while pressed and lighter on hover.
/// \param option Style option of the button.
/// \param painter Painter to draw with.
///
void AppStyle::drawPrimaryButton(const QStyleOption *option, QPainter *painter) const
{
    const bool sunken = option->state.testFlag(State_Sunken) || option->state.testFlag(State_On);
    QColor fill = option->palette.color(QPalette::Accent);
    if (sunken)
        fill = fill.darker(115);
    else if (option->state.testFlag(State_MouseOver))
        fill = fill.lighter(110);

    const QRectF frame = QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5);
    QLinearGradient gradient(frame.topLeft(), frame.bottomLeft());
    gradient.setColorAt(0.0, sunken ? fill : fill.lighter(112));
    gradient.setColorAt(1.0, sunken ? fill.lighter(106) : fill.darker(108));

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(fill.darker(135));
    painter->setBrush(gradient);
    painter->drawRoundedRect(frame, 3, 3);
    if (!sunken) {
        painter->setPen(withAlpha(Qt::white, 60));
        painter->drawLine(QPointF(frame.left() + 3, frame.top() + 1), QPointF(frame.right() - 3, frame.top() + 1));
    }
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

///
/// \brief Draws the icon and the text of a toolbar button centred, with the design gap between them.
/// \param button Button option.
/// \param painter Painter to draw with.
///
void AppStyle::drawToolbarButtonLabel(const QStyleOptionButton *button, QPainter *painter) const
{
    const bool enabled = button->state.testFlag(State_Enabled);
    const int labelWidth = toolbarLabelWidth(button);
    const QRect contents(button->rect.x() + (button->rect.width() - labelWidth) / 2, button->rect.y(),
                         labelWidth, button->rect.height());
    const QRect iconRect = alignedRect(button->direction, Qt::AlignLeft | Qt::AlignVCenter,
                                       button->iconSize, contents);
    const QPixmap pixmap = button->icon.pixmap(button->iconSize, painter->device()->devicePixelRatio(),
                                               enabled ? QIcon::Normal : QIcon::Disabled);
    drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);

    const QRect textRect = visualRect(button->direction, contents,
                                      contents.adjusted(button->iconSize.width() + kToolbarIconGap, 0, 0, 0));
    drawItemText(painter, textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                 button->palette, enabled, button->text, QPalette::ButtonText);
}
