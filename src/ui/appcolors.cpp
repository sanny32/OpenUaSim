// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appcolors.cpp
/// \brief Implements the application's semantic, theme-aware colours.
///

#include <QGuiApplication>
#include <QPalette>

#include "appcolors.h"
#include "apppalette.h"

namespace AppColors {

///
/// \brief Returns the indicator colour of a status for the active palette.
/// \param kind Status to colour.
/// \return Indicator dot colour; transparent for StatusKind::None.
///
QColor status(StatusKind kind)
{
    const bool dark = AppPalette::isDark(QGuiApplication::palette());
    switch (kind) {
    case StatusKind::Good:     return QColor(0x22c55e);
    case StatusKind::Info:     return QColor(0x3b9dff);
    case StatusKind::Warning:  return QColor(0xeab308);
    case StatusKind::Error:    return QColor(0xef4444);
    case StatusKind::Inactive: return dark ? QColor(0x6b7280) : QColor(0x9ca3af);
    case StatusKind::None:     break;
    }
    return Qt::transparent;
}

///
/// \brief Returns a status colour that stays legible as text on the window background.
/// \param kind Status to colour.
/// \return Text colour; the palette text colour for StatusKind::None.
///
QColor statusText(StatusKind kind)
{
    const QPalette palette = QGuiApplication::palette();
    if (kind == StatusKind::None)
        return palette.color(QPalette::WindowText);
    if (AppPalette::isDark(palette))
        return status(kind);

    switch (kind) {
    case StatusKind::Good:    return QColor(0x15803d);
    case StatusKind::Warning: return QColor(0xa16207);
    case StatusKind::Error:   return QColor(0xb91c1c);
    default:                  return status(kind).darker(130);
    }
}

}
