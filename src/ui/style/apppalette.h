// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file apppalette.h
/// \brief Declares the application light and dark palettes.
///

#pragma once

#include <QPalette>

namespace AppPalette {

///
/// \brief Builds the application palette for the Fusion style.
/// \param dark True for the dark appearance, false for the light one.
/// \return Palette with every colour group filled in.
///
QPalette build(bool dark);

///
/// \brief Reports whether a palette is a dark one.
/// \param palette Palette to inspect.
/// \return True when the window background is darker than its text.
///
bool isDark(const QPalette &palette);

}
