// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appstyle.h
/// \brief Declares the application proxy style that draws panels, tabs and accent buttons.
///

#pragma once

#include <QProxyStyle>

class QStyleOptionButton;

///
/// \brief Fusion-based proxy style adding the application's look without changing metrics.
///
/// Widgets opt in through dynamic properties set in the .ui files:
/// - \c panel (bool) on a QFrame draws a rounded card filled with the base colour;
/// - \c panelBody (bool) on a QScrollArea inside a panel lets the panel colour show through;
/// - \c separator (bool) on a one-pixel StyledPanel QFrame draws a line in the mid colour,
///   which stays subtle in both themes unlike QFrame::HLine;
/// - \c accent ("success") on a QPushButton draws a tinted, outlined button; ("primary") fills it with
///   a gradient of the palette accent for the main action of a dialog;
/// - \c toolbar (bool) on a QPushButton, QToolButton or QLineEdit makes it larger, for the main toolbar.
///
/// Tab bars draw separate tabs with the selected one filled in the accent colour, and check
/// boxes draw as accent-filled rounded squares.
///
class AppStyle : public QProxyStyle
{
    Q_OBJECT

public:
    /// \brief Name of the bool property marking a QFrame as a panel.
    static constexpr const char *panelProperty = "panel";
    /// \brief Name of the bool property marking a QScrollArea as the scrolling body of a panel.
    static constexpr const char *panelBodyProperty = "panelBody";
    /// \brief Name of the bool property marking a QFrame as a separator line.
    static constexpr const char *separatorProperty = "separator";
    /// \brief Name of the bool property marking a push button, tool button or line edit of the main toolbar.
    static constexpr const char *toolbarProperty = "toolbar";
    /// \brief Name of the string property selecting a push button accent.
    static constexpr const char *accentProperty = "accent";

    ///
    /// \brief Constructs the style on top of Fusion.
    ///
    AppStyle();

    ///
    /// \brief Draws panel frames, separators, check boxes and accent button bevels; defers
    ///        everything else to Fusion.
    /// \param element Primitive element to draw.
    /// \param option Style option of the element.
    /// \param painter Painter to draw with.
    /// \param widget Widget being drawn.
    ///
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override;

    ///
    /// \brief Draws tab shapes and labels and toolbar and primary button labels; defers everything else to Fusion.
    /// \param element Control element to draw.
    /// \param option Style option of the element.
    /// \param painter Painter to draw with.
    /// \param widget Widget being drawn.
    ///
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget = nullptr) const override;

    ///
    /// \brief Draws toolbar menu buttons with a separate chevron and rounded hover feedback.
    /// \param control Complex control to draw.
    /// \param option Style option of the control.
    /// \param painter Painter to draw with.
    /// \param widget Widget being drawn.
    ///
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget = nullptr) const override;

    ///
    /// \brief Raises tabs, input fields, buttons, header sections and toolbar controls to the design heights.
    /// \param type Contents type being measured.
    /// \param option Style option of the control.
    /// \param contentsSize Size requested by the contents.
    /// \param widget Widget being measured.
    /// \return Adjusted size.
    ///
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &contentsSize,
                           const QWidget *widget = nullptr) const override;

    ///
    /// \brief Sets item view text margins, icon size and tree indentation, header margins and
    ///        check box indicator and message box icon sizes.
    /// \param metric Metric to query.
    /// \param option Style option of the control.
    /// \param widget Widget being measured.
    /// \return Metric value in pixels.
    ///
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;

    ///
    /// \brief Pulls item view text towards a decoration on its left.
    /// \param element Sub-element to place.
    /// \param option Style option of the control.
    /// \param widget Widget being laid out.
    /// \return Sub-element rectangle.
    ///
    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget = nullptr) const override;

    ///
    /// \brief Replaces the platform message box icons with status-coloured badges.
    /// \param standardIcon Icon to return.
    /// \param option Style option of the requesting control.
    /// \param widget Requesting widget.
    /// \return Application icon for message boxes, the Fusion icon otherwise.
    ///
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;

    ///
    /// \brief Adds vertical space to message boxes and makes panel scroll areas transparent.
    /// \param widget Widget being polished.
    ///
    void polish(QWidget *widget) override;

    ///
    /// \brief Starts watching the keyboard so menu mnemonics appear only after Alt, as on Windows.
    /// \param application Application the style is set on.
    ///
    void polish(QApplication *application) override;

    ///
    /// \brief Stops watching the keyboard.
    /// \param application Application the style is removed from.
    ///
    void unpolish(QApplication *application) override;

    using QProxyStyle::polish;
    using QProxyStyle::unpolish;

    ///
    /// \brief Underlines mnemonics only while keyboard cues are on; defers everything else to Fusion.
    /// \param hint Style hint to query.
    /// \param option Style option of the control.
    /// \param widget Widget being queried.
    /// \param returnData Extra data the hint may return.
    /// \return Hint value.
    ///
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;

    ///
    /// \brief Turns keyboard cues on when Alt is pressed and off on a mouse press.
    /// \param watched Object receiving the event.
    /// \param event Event being delivered.
    /// \return False, so the event is always delivered.
    ///
    bool eventFilter(QObject *watched, QEvent *event) override;

    ///
    /// \brief Returns the radius of panel and tab corners.
    /// \return Radius in pixels.
    ///
    static constexpr int cornerRadius() { return 6; }

private:
    void drawPanel(const QStyleOption *option, QPainter *painter) const;
    void drawCheckIndicator(const QStyleOption *option, QPainter *painter) const;
    void drawAccentButton(const QStyleOption *option, QPainter *painter, const QColor &accent) const;
    void drawPrimaryButton(const QStyleOption *option, QPainter *painter) const;
    void drawTabShape(const QStyleOption *option, QPainter *painter) const;
    void drawToolbarButtonLabel(const QStyleOptionButton *button, QPainter *painter) const;
    void setKeyboardCues(bool on);

    bool _keyboardCues = false;
};
