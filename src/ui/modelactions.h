// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modelactions.h
/// \brief Declares the user actions on NodeSet2 models: import, export and removal.
///

#pragma once

#include <QObject>

class NamespacesPage;
class ServerController;
class QWidget;

///
/// \brief Carries out the model requests of the menus and the Namespaces page, asking before a model is removed
///        and reporting failures in message boxes.
///
class ModelActions : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Wires the Namespaces page to the controller.
    /// \param controller Controller serving the models; must outlive the actions.
    /// \param page Page whose requests are carried out.
    /// \param window Parent of the dialogs and owner of the actions.
    ///
    ModelActions(ServerController &controller, NamespacesPage *page, QWidget *window);

    ///
    /// \brief Asks for a NodeSet2 file and imports it.
    ///
    void importModel();

    ///
    /// \brief Asks for a file name and exports the project nodes there.
    ///
    void exportModel();

    ///
    /// \brief Imports a NodeSet2 model, reporting a file that cannot be loaded.
    /// \param fileName NodeSet2 file.
    /// \return True when the model was imported.
    ///
    bool importModelFrom(const QString &fileName);

    ///
    /// \brief Exports the project nodes as NodeSet2, reporting a file that cannot be written.
    /// \param fileName Target path.
    /// \return True when the file was written.
    ///
    bool exportModelTo(const QString &fileName);

    ///
    /// \brief Exports the nodes of one namespace as NodeSet2, reporting a file that cannot be written.
    /// \param fileName Target path.
    /// \param namespaceUri URI of the namespace to export.
    /// \return True when the file was written.
    ///
    bool exportNamespaceTo(const QString &fileName, const QString &namespaceUri);

private:
    void exportNamespace(const QString &namespaceUri);
    void confirmRemoveModel(const QString &fileName);
    void removePendingModel();

    ServerController &_controller;
    QWidget *_window;
    QString _pendingModelFile;
};
