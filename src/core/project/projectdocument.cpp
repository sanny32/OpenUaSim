// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectdocument.cpp
/// \brief Implements the project file the controller serves: its name, unsaved changes and persistence.
///

#include <QFileInfo>
#include <QSettings>

#include "defaultproject.h"
#include "projectdocument.h"
#include "projectfile.h"
#include "server/servercontroller.h"

namespace {

const QString kLastFileKey = QStringLiteral("project/lastFile");

}

///
/// \brief Creates a document for a controller.
/// \param controller Controller serving the document's project; must outlive the document.
/// \param parent Owning QObject.
///
ProjectDocument::ProjectDocument(ServerController *controller, QObject *parent)
    : QObject(parent)
    , _controller(controller)
{
    connect(controller, &ServerController::projectEdited, this, &ProjectDocument::markModified);
}

///
/// \brief Serves the default project without a file.
///
void ProjectDocument::newProject()
{
    serve(QString(), DefaultProject::create());
}

///
/// \brief Serves a project file; a running server restarts with it.
/// \param fileName File to read.
/// \param error Receives the reason on failure; may be null.
/// \return False when the file cannot be read; the served project is kept then.
///
bool ProjectDocument::open(const QString &fileName, QString *error)
{
    const ProjectFile::LoadResult result = ProjectFile::load(fileName);
    if (!result.ok()) {
        if (error)
            *error = result.error;
        return false;
    }
    serve(fileName, result.project);
    rememberFile(fileName);
    return true;
}

///
/// \brief Opens the file opened or saved last, or serves the default project.
/// \return True when the last file was opened.
///
bool ProjectDocument::openLast()
{
    const QString last = QSettings().value(kLastFileKey).toString();
    if (!last.isEmpty() && open(last))
        return true;
    newProject();
    return false;
}

///
/// \brief Writes the served project to the document's file.
/// \param error Receives the reason on failure; may be null.
/// \return False when the document has no file or the file cannot be written.
///
bool ProjectDocument::save(QString *error)
{
    if (_fileName.isEmpty()) {
        if (error)
            *error = tr("The project has no file yet");
        return false;
    }
    return saveAs(_fileName, error);
}

///
/// \brief Writes the served project to a new file and makes it the document's file.
/// \param fileName Target path.
/// \param error Receives the reason on failure; may be null.
/// \return False when the file cannot be written.
///
bool ProjectDocument::saveAs(const QString &fileName, QString *error)
{
    if (!ProjectFile::save(_controller->project(), fileName, error))
        return false;
    setFileName(fileName);
    setModified(false);
    rememberFile(fileName);
    return true;
}

///
/// \brief Returns the document's file.
/// \return Path, or empty for a project never saved.
///
QString ProjectDocument::fileName() const
{
    return _fileName;
}

///
/// \brief Returns the name to show in the window title.
/// \return File name without directory, or "Untitled".
///
QString ProjectDocument::displayName() const
{
    return _fileName.isEmpty() ? tr("Untitled") : QFileInfo(_fileName).fileName();
}

///
/// \brief Tells whether the project has edits not written to its file.
/// \return True after an edit until the next save, open or new project.
///
bool ProjectDocument::isModified() const
{
    return _modified;
}

///
/// \brief Hands a project to the controller, restarting a running server so it serves the new endpoints.
/// \param fileName File the project came from; empty for none.
/// \param project Project to serve.
///
void ProjectDocument::serve(const QString &fileName, const ProjectConfig &project)
{
    const bool wasRunning = _controller->isRunning();
    _controller->stop();
    _controller->setProject(project);
    if (wasRunning)
        _controller->start();
    setFileName(fileName);
    setModified(false);
}

///
/// \brief Stores the file name and announces a change.
/// \param fileName New path.
///
void ProjectDocument::setFileName(const QString &fileName)
{
    if (_fileName == fileName)
        return;
    _fileName = fileName;
    emit fileNameChanged(fileName);
}

///
/// \brief Stores whether edits are unsaved and announces a change.
/// \param modified New state.
///
void ProjectDocument::setModified(bool modified)
{
    if (_modified == modified)
        return;
    _modified = modified;
    emit modifiedChanged(modified);
}

///
/// \brief Records an edit of the served project.
///
void ProjectDocument::markModified()
{
    setModified(true);
}

///
/// \brief Remembers a file for openLast().
/// \param fileName Path opened or saved.
///
void ProjectDocument::rememberFile(const QString &fileName) const
{
    QSettings().setValue(kLastFileKey, QFileInfo(fileName).absoluteFilePath());
}
