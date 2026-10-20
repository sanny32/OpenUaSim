// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectmodels.h
/// \brief Declares the directory that keeps copies of the imported models next to a project file.
///

#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

namespace ProjectModels {

///
/// \brief Returns the directory keeping the models of a project file.
/// \param projectFile Project file path.
/// \return Absolute path such as "/work/plant.models" for "/work/plant.ouasim".
///
QString directory(const QString &projectFile);

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
           QString *error = nullptr);

}
