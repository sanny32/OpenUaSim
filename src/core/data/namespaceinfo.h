// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file namespaceinfo.h
/// \brief Declares the description of a registered namespace.
///

#pragma once

#include <QDate>
#include <QString>

///
/// \brief Describes one entry of the server namespace array.
///
struct NamespaceInfo
{
    /// \brief Index in the namespace array.
    int index = 0;
    /// \brief Namespace URI.
    QString uri;
    /// \brief Model version.
    QString version;
    /// \brief Model publication date.
    QDate publicationDate;
    /// \brief Free-form description.
    QString description;
};
