// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file apppalette.cpp
/// \brief Implements the application light and dark palettes.
///

#include <QColor>

#include "apppalette.h"

namespace AppPalette {

namespace {

///
/// \brief Base colours a palette is derived from.
///
struct Scheme
{
    QColor window;
    QColor base;
    QColor alternateBase;
    QColor button;
    QColor text;
    QColor disabledText;
    QColor border;
    QColor highlight;
    QColor highlightedText;
    QColor link;
    QColor toolTipBase;
};

///
/// \brief Returns the dark scheme: navy window, slightly lighter panels, blue accent.
/// \return Dark base colours.
///
Scheme darkScheme()
{
    return {
        QColor(0x0e141e), QColor(0x141b27), QColor(0x19212e), QColor(0x1c2533),
        QColor(0xe5e7eb), QColor(0x6b7280), QColor(0x2b3546), QColor(0x1d63c9),
        QColor(0xffffff), QColor(0x60a5fa), QColor(0x1f2937),
    };
}

///
/// \brief Returns the light scheme: neutral grey surfaces with a blue accent.
/// \return Light base colours.
///
Scheme lightScheme()
{
    return {
        QColor(0xf1f3f6), QColor(0xffffff), QColor(0xf7f8fa), QColor(0xfbfbfc),
        QColor(0x111827), QColor(0x9ca3af), QColor(0xd4d8de), QColor(0x2563eb),
        QColor(0xffffff), QColor(0x2563eb), QColor(0xffffff),
    };
}

}

///
/// \brief Builds the application palette for the Fusion style.
/// \param dark True for the dark appearance, false for the light one.
/// \return Palette with every colour group filled in.
///
QPalette build(bool dark)
{
    const Scheme scheme = dark ? darkScheme() : lightScheme();

    QColor placeholder = scheme.text;
    placeholder.setAlpha(110);

    QPalette palette;
    palette.setColor(QPalette::Window, scheme.window);
    palette.setColor(QPalette::WindowText, scheme.text);
    palette.setColor(QPalette::Base, scheme.base);
    palette.setColor(QPalette::AlternateBase, scheme.alternateBase);
    palette.setColor(QPalette::Text, scheme.text);
    palette.setColor(QPalette::Button, scheme.button);
    palette.setColor(QPalette::ButtonText, scheme.text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Light, dark ? scheme.border.lighter(130) : Qt::white);
    palette.setColor(QPalette::Midlight, dark ? scheme.border.lighter(115) : scheme.border.lighter(108));
    palette.setColor(QPalette::Mid, scheme.border);
    palette.setColor(QPalette::Dark, dark ? scheme.base.darker(130) : scheme.border.darker(120));
    palette.setColor(QPalette::Shadow, dark ? QColor(0x05080d) : scheme.border.darker(150));
    palette.setColor(QPalette::Highlight, scheme.highlight);
    palette.setColor(QPalette::HighlightedText, scheme.highlightedText);
    palette.setColor(QPalette::Accent, scheme.highlight);
    palette.setColor(QPalette::Link, scheme.link);
    palette.setColor(QPalette::LinkVisited, scheme.link);
    palette.setColor(QPalette::ToolTipBase, scheme.toolTipBase);
    palette.setColor(QPalette::ToolTipText, scheme.text);
    palette.setColor(QPalette::PlaceholderText, placeholder);

    for (QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
        palette.setColor(QPalette::Disabled, role, scheme.disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Base, scheme.window);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, scheme.border);
    palette.setColor(QPalette::Inactive, QPalette::Highlight, scheme.highlight);
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText, scheme.highlightedText);
    return palette;
}

///
/// \brief Reports whether a palette is a dark one.
/// \param palette Palette to inspect.
/// \return True when the window background is darker than its text.
///
bool isDark(const QPalette &palette)
{
    return palette.color(QPalette::Window).lightness() < palette.color(QPalette::WindowText).lightness();
}

}
