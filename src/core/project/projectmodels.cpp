// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectmodels.cpp
/// \brief Implements the directory that keeps copies of the imported models next to a project file.
///

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include "projectmodels.h"

namespace {

///
/// \brief Translates a message about project models.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("ProjectModels", text);
}

///
/// \brief Picks a file name not used yet in the models directory.
/// \param fileName Name of the model file, e.g. "boiler.NodeSet2.xml".
/// \param taken Names already used, compared case-insensitively.
/// \return The name itself, or one with a numeric suffix such as "boiler_2.NodeSet2.xml".
///
QString freeName(const QString &fileName, const QSet<QString> &taken)
{
    const QFileInfo info(fileName);
    QString name = fileName;
    for (int suffix = 2; taken.contains(name.toLower()); ++suffix)
        name = QStringLiteral("%1_%2.%3").arg(info.baseName()).arg(suffix).arg(info.completeSuffix());
    return name;
}

///
/// \brief Records why storing the models failed.
/// \param error Receives the reason; may be null.
/// \param message Reason.
/// \return Always false, so callers can return it directly.
///
bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

}

namespace ProjectModels {

///
/// \brief Returns the directory keeping the models of a project file.
/// \param projectFile Project file path.
/// \return Absolute path such as "/work/plant.models" for "/work/plant.ouasim".
///
QString directory(const QString &projectFile)
{
    const QFileInfo info(projectFile);
    return info.absoluteDir().absoluteFilePath(info.completeBaseName() + QStringLiteral(".models"));
}

///
/// \brief Copies the models of a project into its models directory and deletes the model files there that the
///        project no longer imports.
/// \param models Absolute paths of the imported models.
/// \param projectFile Project file the models are kept next to.
/// \param movedModels Receives the new absolute path keyed by the former one for every copied model; models
///        already in the directory are kept and not listed.
/// \param error Receives the reason on failure; may be null.
/// \return False when the directory cannot be created or a model cannot be copied.
///
bool store(const QStringList &models, const QString &projectFile, QHash<QString, QString> &movedModels,
           QString *error)
{
    QDir target(directory(projectFile));
    if (!models.isEmpty() && !target.mkpath(QStringLiteral(".")))
        return fail(error, tr("Cannot create the models directory '%1'").arg(target.absolutePath()));

    QSet<QString> kept;
    QStringList copied;
    for (const QString &model : models) {
        if (QFileInfo(model).absoluteDir() == target)
            kept.insert(QFileInfo(model).fileName().toLower());
        else
            copied.append(model);
    }
    for (const QString &model : std::as_const(copied)) {
        const QString name = freeName(QFileInfo(model).fileName(), kept);
        const QString copy = target.absoluteFilePath(name);
        if (QFile::exists(copy) && !QFile::remove(copy))
            return fail(error, tr("Cannot replace '%1'").arg(copy));
        if (!QFile::copy(model, copy))
            return fail(error, tr("Cannot copy the model '%1' to '%2'").arg(model, copy));
        kept.insert(name.toLower());
        movedModels.insert(model, copy);
    }

    if (!target.exists())
        return true;
    const QStringList stored = target.entryList({ QStringLiteral("*.xml") }, QDir::Files);
    for (const QString &name : stored) {
        if (!kept.contains(name.toLower()))
            QFile::remove(target.absoluteFilePath(name));
    }
    if (target.isEmpty())
        QDir().rmdir(target.absolutePath());
    return true;
}

}
