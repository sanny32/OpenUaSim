// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectfile.h
/// \brief Declares reading and writing of OpenUaSim project files.
///

#pragma once

#include <QByteArray>
#include <QString>

#include "data/projectconfig.h"

namespace ProjectFile {

/// \brief Version written to new files; files of a newer version are refused.
constexpr int kFormatVersion = 2;

///
/// \brief Outcome of reading a project.
///
struct LoadResult
{
    /// \brief Project read; default-constructed when reading failed.
    ProjectConfig project;
    /// \brief Reason reading failed; empty on success.
    QString error;

    ///
    /// \brief Tells whether the project was read.
    /// \return True when error is empty.
    ///
    bool ok() const { return error.isEmpty(); }
};

///
/// \brief Returns the file name filter of project files for file dialogs.
/// \return Filter such as "OpenUaSim project (*.ouasim)".
///
QString fileFilter();

///
/// \brief Serializes a project to JSON.
/// \param project Project to write.
/// \return Indented JSON document.
///
QByteArray toJson(const ProjectConfig &project);

///
/// \brief Parses a project from JSON and validates it.
/// \param json Document produced by toJson().
/// \return Project, or the first problem found: malformed JSON, a newer format version,
///         an unknown node kind, data type, access level or simulation function, a duplicated
///         node id, or model variable settings without a namespace URI or given twice.
///
LoadResult fromJson(const QByteArray &json);

///
/// \brief Writes a project to a file, replacing it atomically.
/// \param project Project to write; its model paths are stored relative to the file's directory.
/// \param fileName Target path.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written.
///
bool save(const ProjectConfig &project, const QString &fileName, QString *error = nullptr);

///
/// \brief Reads a project from a file.
/// \param fileName Path to read.
/// \return Project with absolute model paths, or the reason the file could not be opened or parsed.
///
LoadResult load(const QString &fileName);

}
