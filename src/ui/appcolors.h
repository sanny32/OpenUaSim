// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appcolors.h
/// \brief Declares the application's semantic, theme-aware colours.
///

#pragma once

#include <QColor>
#include <QPalette>

#include "data/statuskind.h"

namespace AppColors {

///
/// \brief Returns the indicator colour of a status for the active palette.
/// \param kind Status to colour.
/// \return Indicator dot colour; transparent for StatusKind::None.
///
QColor status(StatusKind kind);

///
/// \brief Returns a status colour that stays legible as text on the window background.
/// \param kind Status to colour.
/// \return Text colour; the palette text colour for StatusKind::None.
///
QColor statusText(StatusKind kind);

///
/// \brief Returns the colour of supporting text, quieter than the main text but still legible.
/// \param palette Palette whose text and window colours are blended.
/// \return Opaque text colour.
///
QColor secondaryText(const QPalette &palette);

}
