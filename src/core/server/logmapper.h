// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logmapper.h
/// \brief Declares the conversion of QUaServer log messages into server log entries.
///

#pragma once

#include <optional>

#include "data/logentry.h"

struct QUaLog;

namespace LogMapper {

///
/// \brief Converts a QUaServer log message.
/// \param log Message emitted by QUaServer::logMessage().
/// \return Entry with the mapped level and category, or nothing for trace and debug messages.
///
std::optional<LogEntry> fromQUaLog(const QUaLog &log);

///
/// \brief Builds an entry produced by the simulator itself.
/// \param level Severity.
/// \param category Producing subsystem.
/// \param message Text.
/// \return Entry stamped with the current time.
///
LogEntry entry(LogEntry::Level level, LogEntry::Category category, const QString &message);

}
