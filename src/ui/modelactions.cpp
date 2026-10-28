// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modelactions.cpp
/// \brief Implements the user actions on NodeSet2 models: import, export and removal.
///

#include <QFileDialog>
#include <QFileInfo>

#include "messagedialog.h"
#include "modelactions.h"
#include "namespacespage.h"
#include "recentdirectories.h"
#include "server/servercontroller.h"

namespace {

///
/// \brief Returns the file name filter of NodeSet2 files for file dialogs.
/// \return Filter such as "NodeSet2 XML (*.xml)".
///
QString nodeSetFilter()
{
    return ModelActions::tr("NodeSet2 XML (*.xml)");
}

}

///
/// \brief Wires the Namespaces page to the controller.
/// \param controller Controller serving the models; must outlive the actions.
/// \param page Page whose requests are carried out.
/// \param window Parent of the dialogs and owner of the actions.
///
ModelActions::ModelActions(ServerController &controller, NamespacesPage *page, QWidget *window)
    : QObject(window)
    , _controller(controller)
    , _window(window)
{
    connect(page, &NamespacesPage::importRequested, this, &ModelActions::importModel);
    connect(page, &NamespacesPage::exportRequested, this, &ModelActions::exportNamespace);
    connect(page, &NamespacesPage::modelRemovalRequested, this, &ModelActions::confirmRemoveModel);
}

///
/// \brief Asks for a NodeSet2 file and imports it.
///
void ModelActions::importModel()
{
    const QString fileName = QFileDialog::getOpenFileName(
        _window, tr("Import Model"), RecentDirectories::directory(RecentDirectories::Purpose::Model), nodeSetFilter());
    if (fileName.isEmpty())
        return;
    RecentDirectories::remember(RecentDirectories::Purpose::Model, fileName);
    importModelFrom(fileName);
}

///
/// \brief Asks for a file name and exports the project nodes there.
///
void ModelActions::exportModel()
{
    const QString fileName = QFileDialog::getSaveFileName(_window, tr("Export Model"), QString(), nodeSetFilter());
    if (!fileName.isEmpty())
        exportModelTo(fileName);
}

///
/// \brief Imports a NodeSet2 model, reporting a file that cannot be loaded.
/// \param fileName NodeSet2 file.
/// \return True when the model was imported.
///
bool ModelActions::importModelFrom(const QString &fileName)
{
    QString error;
    if (_controller.importNodeSet(fileName, &error))
        return true;
    MessageDialog::warning(tr("Model not imported"), error, _window);
    return false;
}

///
/// \brief Exports the project nodes as NodeSet2, reporting a file that cannot be written.
/// \param fileName Target path.
/// \return True when the file was written.
///
bool ModelActions::exportModelTo(const QString &fileName)
{
    QString error;
    if (_controller.exportNodeSet(fileName, &error))
        return true;
    MessageDialog::warning(tr("Model not exported"), error, _window);
    return false;
}

///
/// \brief Exports the nodes of one namespace as NodeSet2, reporting a file that cannot be written.
/// \param fileName Target path.
/// \param namespaceUri URI of the namespace to export.
/// \return True when the file was written.
///
bool ModelActions::exportNamespaceTo(const QString &fileName, const QString &namespaceUri)
{
    QString error;
    if (_controller.exportNamespace(fileName, namespaceUri, &error))
        return true;
    MessageDialog::warning(tr("Namespace not exported"), error, _window);
    return false;
}

///
/// \brief Asks for a file name and exports the nodes of a namespace there.
/// \param namespaceUri URI of the namespace to export.
///
void ModelActions::exportNamespace(const QString &namespaceUri)
{
    const QString fileName = QFileDialog::getSaveFileName(_window, tr("Export Namespace"), QString(),
                                                          nodeSetFilter());
    if (!fileName.isEmpty())
        exportNamespaceTo(fileName, namespaceUri);
}

///
/// \brief Asks before unloading an imported model.
/// \param fileName Model file.
///
void ModelActions::confirmRemoveModel(const QString &fileName)
{
    _pendingModelFile = fileName;
    auto *dialog = MessageDialog::confirmation(
        tr("Remove Model"),
        tr("Remove the model '%1' with its nodes and namespaces?").arg(QFileInfo(fileName).fileName()),
        tr("A running server restarts and disconnects its clients."), tr("Remove"), _window);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &ModelActions::removePendingModel);
    dialog->open();
}

///
/// \brief Unloads the model the Remove Model question was confirmed for.
///
void ModelActions::removePendingModel()
{
    QString error;
    if (!_controller.removeNodeSet(_pendingModelFile, &error))
        MessageDialog::warning(tr("Model not removed"), error, _window);
}
