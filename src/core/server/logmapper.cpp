// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logmapper.cpp
/// \brief Implements the conversion of QUaServer log messages into server log entries.
///

#include <QMetaEnum>

#include <QUaServer>

#include "logmapper.h"

namespace {

///
/// \brief Maps a QUaServer category onto a log page category.
/// \param category QUaServer category.
/// \return Session, Security or Server.
///
LogEntry::Category category(QUaLogCategory category)
{
    switch (category) {
    case QUaLogCategory::Session:
        return LogEntry::Category::Session;
    case QUaLogCategory::SecureChannel:
    case QUaLogCategory::SecurityPolicy:
        return LogEntry::Category::Security;
    default:
        return LogEntry::Category::Server;
    }
}

}

namespace LogMapper {

///
/// \brief Converts a QUaServer log message.
/// \param log Message emitted by QUaServer::logMessage().
/// \return Entry with the mapped level and category, or nothing for trace and debug messages.
///
std::optional<LogEntry> fromQUaLog(const QUaLog &log)
{
    LogEntry::Level level = LogEntry::Level::Info;
    switch (log.level) {
    case QUaLogLevel::Trace:
    case QUaLogLevel::Debug:
        return std::nullopt;
    case QUaLogLevel::Info:
        level = LogEntry::Level::Info;
        break;
    case QUaLogLevel::Warning:
        level = LogEntry::Level::Warning;
        break;
    case QUaLogLevel::Error:
    case QUaLogLevel::Fatal:
        level = LogEntry::Level::Error;
        break;
    }

    LogEntry entry;
    entry.time = log.timestamp.isValid() ? log.timestamp : QDateTime::currentDateTime();
    entry.level = level;
    entry.category = category(log.category);
    entry.message = QString::fromUtf8(log.message);
    entry.source = QString::fromLatin1(QMetaEnum::fromType<QUaLogCategory>().valueToKey(int(log.category)));
    return entry;
}

///
/// \brief Builds an entry produced by the simulator itself.
/// \param level Severity.
/// \param category Producing subsystem.
/// \param message Text.
/// \return Entry stamped with the current time.
///
LogEntry entry(LogEntry::Level level, LogEntry::Category category, const QString &message)
{
    LogEntry result;
    result.time = QDateTime::currentDateTime();
    result.level = level;
    result.category = category;
    result.message = message;
    result.source = QStringLiteral("OpenUaSim");
    return result;
}

}
