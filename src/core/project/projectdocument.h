// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectdocument.h
/// \brief Declares the project file the controller serves: its name, unsaved changes and persistence.
///

#pragma once

#include <QObject>
#include <QString>

class ServerController;
struct ProjectConfig;

///
/// \brief Ties the served project to a file and remembers the last one opened or saved.
///
class ProjectDocument : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Creates a document for a controller.
    /// \param controller Controller serving the document's project; must outlive the document.
    /// \param parent Owning QObject.
    ///
    explicit ProjectDocument(ServerController *controller, QObject *parent = nullptr);

    ///
    /// \brief Serves the default project without a file.
    ///
    void newProject();

    ///
    /// \brief Serves a project file; a running server restarts with it.
    /// \param fileName File to read.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the file cannot be read; the served project is kept then.
    ///
    bool open(const QString &fileName, QString *error = nullptr);

    ///
    /// \brief Opens the file opened or saved last, or serves the default project.
    /// \return True when the last file was opened.
    ///
    bool openLast();

    ///
    /// \brief Writes the served project to the document's file.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the document has no file or the file cannot be written.
    ///
    bool save(QString *error = nullptr);

    ///
    /// \brief Writes the served project to a new file and makes it the document's file.
    /// \param fileName Target path.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the file cannot be written.
    ///
    bool saveAs(const QString &fileName, QString *error = nullptr);

    ///
    /// \brief Returns the document's file.
    /// \return Path, or empty for a project never saved.
    ///
    QString fileName() const;

    ///
    /// \brief Returns the name to show in the window title.
    /// \return File name without directory, or "Untitled".
    ///
    QString displayName() const;

    ///
    /// \brief Tells whether the project has edits not written to its file.
    /// \return True after an edit until the next save, open or new project.
    ///
    bool isModified() const;

signals:
    ///
    /// \brief Announces a new file name, e.g. after Open or Save As.
    /// \param fileName New path; empty for a new project.
    ///
    void fileNameChanged(const QString &fileName);

    ///
    /// \brief Announces that unsaved changes appeared or were saved.
    /// \param modified New state.
    ///
    void modifiedChanged(bool modified);

private:
    void serve(const QString &fileName, const ProjectConfig &project);
    void setFileName(const QString &fileName);
    void setModified(bool modified);
    void markModified();
    void rememberFile(const QString &fileName) const;

    ServerController *_controller;
    QString _fileName;
    bool _modified = false;
};
