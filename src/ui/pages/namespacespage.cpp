// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacespage.cpp
/// \brief Implements the namespaces page.
///

#include "appicons.h"
#include "itemroles.h"
#include "namespacesmodel.h"
#include "namespacespage.h"
#include "treebuilders.h"
#include "treemodel.h"
#include "ui_namespacespage.h"
#include "viewsetup.h"

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
NamespacesPage::NamespacesPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NamespacesPage)
    , _treeModel(new TreeModel(this))
    , _namespacesModel(new NamespacesModel(this))
{
    ui->setupUi(this);
    ViewSetup::bindCopyButton(ui->copyIndexButton, ui->indexEdit);
    ViewSetup::bindCopyButton(ui->copyUriButton, ui->uriEdit);
    ui->importButton->setIcon(AppIcons::icon(QStringLiteral("download")));
    ui->exportButton->setIcon(AppIcons::icon(QStringLiteral("upload")));

    ViewSetup::installDelegate(ui->namespacesTree);
    ViewSetup::attachSearch(ui->namespacesTree, _treeModel, ui->searchEdit);
    ViewSetup::configureTable(ui->namespacesView);
    ui->namespacesView->setModel(_namespacesModel);

    connect(ui->namespacesTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &NamespacesPage::showTreeNode);
    connect(ui->namespacesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &NamespacesPage::showNamespace);
}

///
/// \brief Destroys the page.
///
NamespacesPage::~NamespacesPage()
{
    delete ui;
}

///
/// \brief Shows a namespace array.
/// \param namespaces Registered namespaces.
///
void NamespacesPage::setNamespaces(const QList<NamespaceInfo> &namespaces)
{
    _namespacesModel->setItems(namespaces);
    _treeModel->setNodes(TreeBuilders::namespaceTree(namespaces));
    ViewSetup::applyInitialState(ui->namespacesTree);
    ViewSetup::selectRow(ui->namespacesView, 0);
}

///
/// \brief Returns the namespaces table model.
/// \return Model of the registered namespaces.
///
const NamespacesModel *NamespacesPage::namespacesModel() const
{
    return _namespacesModel;
}

///
/// \brief Selects the table row of the namespace picked in the tree.
/// \param current Selected tree index.
///
void NamespacesPage::showTreeNode(const QModelIndex &current)
{
    const QString uri = current.data(ItemRoles::Id).toString();
    if (uri.isEmpty())
        return;
    const auto &items = _namespacesModel->items();
    for (int row = 0; row < items.size(); ++row) {
        if (items.at(row).uri == uri) {
            ViewSetup::selectRow(ui->namespacesView, row);
            return;
        }
    }
}

///
/// \brief Fills the details panel from the selected namespace.
/// \param current Selected table index.
///
void NamespacesPage::showNamespace(const QModelIndex &current)
{
    ui->detailsHost->setEnabled(current.isValid());
    if (!current.isValid())
        return;
    const NamespaceInfo &ns = _namespacesModel->item(current.row());
    ui->indexEdit->setText(QString::number(ns.index));
    ui->uriEdit->setText(ns.uri);
    ui->versionEdit->setText(ns.version);
    ui->publicationDateEdit->setDate(ns.publicationDate);
    ui->descriptionEdit->setPlainText(ns.description);
}
