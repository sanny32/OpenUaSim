// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logentry.h
/// \brief Declares one entry of the server log.
///

#pragma once

#include <QDateTime>
#include <QString>

///
/// \brief Describes one server log entry.
///
struct LogEntry
{
    ///
    /// \brief Severity of the entry.
    ///
    enum class Level { Info, Warning, Error };

    ///
    /// \brief Subsystem that produced the entry.
    ///
    enum class Category { Server, Session, Security, DataChanges };

    /// \brief Time the entry was produced.
    QDateTime time;
    /// \brief Severity.
    Level level = Level::Info;
    /// \brief Producing subsystem.
    Category category = Category::Server;
    /// \brief Message text.
    QString message;
    /// \brief Component name.
    QString source;
};
