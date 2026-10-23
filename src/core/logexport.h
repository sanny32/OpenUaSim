// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file logexport.h
/// \brief Declares the export of log entries to CSV.
///

#pragma once

#include <QList>
#include <QString>

#include "data/logentry.h"

namespace LogExport {

///
/// \brief Formats log entries as CSV with a header row.
/// \param entries Entries to format, in output order.
/// \return CSV text with the columns Time, Level, Category, Source and Message.
///
QString toCsv(const QList<LogEntry> &entries);

///
/// \brief Writes log entries to a CSV file, replacing its content.
/// \param fileName Target path.
/// \param entries Entries to write.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written.
///
bool write(const QString &fileName, const QList<LogEntry> &entries, QString *error = nullptr);

}
