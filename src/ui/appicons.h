// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appicons.h
/// \brief Declares the theme-aware application icons.
///

#pragma once

#include <QIcon>
#include <QString>

#include "data/statuskind.h"

namespace AppIcons {

///
/// \brief Builds an icon whose outline follows the palette text colour.
///
/// Monochrome icons draw with \c currentColor, which is resolved each time the icon is
/// rendered, so the icon follows a theme switch without being rebuilt.
///
/// \param name Resource name under ":/icons/", without the ".svg" extension.
/// \return Icon; null when the resource does not exist.
///
QIcon icon(const QString &name);

///
/// \brief Builds the round indicator icon of a status.
/// \param kind Status to show.
/// \return Filled dot in the status colour; null for StatusKind::None.
///
QIcon statusDot(StatusKind kind);

}
