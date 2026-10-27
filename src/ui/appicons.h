// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appicons.h
/// \brief Declares the theme-aware application icons.
///

#pragma once

#include <QIcon>
#include <QList>
#include <QString>
#include <QStyle>

#include "data/statuskind.h"

namespace AppIcons {

///
/// \brief Icon resource offered for selection, with its user-visible name.
///
struct NamedIcon
{
    /// \brief Resource name under ":/icons/", without the ".svg" extension.
    QString name;
    /// \brief Translated name shown in pickers.
    QString label;
};

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
/// \brief Builds an icon whose outline takes the colour of a status, and the disabled text colour when disabled.
/// \param name Resource name under ":/icons/", without the ".svg" extension.
/// \param kind Status giving the colour.
/// \return Icon; null when the resource does not exist.
///
QIcon statusIcon(const QString &name, StatusKind kind);

///
/// \brief Builds the round indicator icon of a status.
/// \param kind Status to show.
/// \return Filled dot in the status colour; null for StatusKind::None.
///
QIcon statusDot(StatusKind kind);

///
/// \brief Builds a message box icon: a status-coloured badge with a glyph.
/// \param pixmap SP_MessageBoxInformation, SP_MessageBoxQuestion, SP_MessageBoxWarning or SP_MessageBoxCritical.
/// \return Icon; null for any other pixmap.
///
QIcon messageIcon(QStyle::StandardPixmap pixmap);

///
/// \brief Lists the icons a variable can be given.
/// \return Icons in picker order.
///
QList<NamedIcon> variableIcons();

}
