// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logexport.cpp
/// \brief Implements the export of log entries to CSV.
///

#include <QCoreApplication>
#include <QSaveFile>
#include <QStringList>

#include "formatting.h"
#include "logexport.h"
#include "logmodel.h"

namespace LogExport {

namespace {

///
/// \brief Quotes a CSV field when it holds a separator, a quote or a line break (RFC 4180).
/// \param field Raw field text.
/// \return Field text safe to place between commas.
///
QString csvField(const QString &field)
{
    if (!field.contains(QLatin1Char(',')) && !field.contains(QLatin1Char('"'))
        && !field.contains(QLatin1Char('\n')) && !field.contains(QLatin1Char('\r')))
        return field;
    QString quoted = field;
    quoted.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QLatin1Char('"') + quoted + QLatin1Char('"');
}

///
/// \brief Joins fields into one CSV record terminated by CRLF.
/// \param fields Raw field texts.
/// \return CSV record.
///
QString csvRecord(const QStringList &fields)
{
    QStringList quoted;
    quoted.reserve(fields.size());
    for (const QString &field : fields)
        quoted.append(csvField(field));
    return quoted.join(QLatin1Char(',')) + QStringLiteral("\r\n");
}

}

///
/// \brief Formats log entries as CSV with a header row.
/// \param entries Entries to format, in output order.
/// \return CSV text with the columns Time, Level, Category, Source, Thread, Event ID and Message.
///
QString toCsv(const QList<LogEntry> &entries)
{
    QString csv = csvRecord({
        QCoreApplication::translate("LogExport", "Time"),
        QCoreApplication::translate("LogExport", "Level"),
        QCoreApplication::translate("LogExport", "Category"),
        QCoreApplication::translate("LogExport", "Source"),
        QCoreApplication::translate("LogExport", "Thread"),
        QCoreApplication::translate("LogExport", "Event ID"),
        QCoreApplication::translate("LogExport", "Message"),
    });
    for (const LogEntry &entry : entries) {
        csv += csvRecord({
            Formatting::preciseDateTime(entry.time),
            LogModel::levelText(entry.level),
            LogModel::categoryText(entry.category),
            entry.source,
            entry.thread,
            QString::number(entry.eventId),
            entry.message,
        });
    }
    return csv;
}

///
/// \brief Writes log entries to a CSV file, replacing its content.
/// \param fileName Target path.
/// \param entries Entries to write.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written.
///
bool write(const QString &fileName, const QList<LogEntry> &entries, QString *error)
{
    QSaveFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        // The byte order mark makes spreadsheet applications read the file as UTF-8.
        file.write("\xEF\xBB\xBF");
        file.write(toCsv(entries).toUtf8());
        if (file.commit())
            return true;
    }
    if (error)
        *error = file.errorString();
    return false;
}

}
