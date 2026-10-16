// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacespage.cpp
/// \brief Implements the namespaces page.
///

#include <QSortFilterProxyModel>

#include "appicons.h"
#include "namespacesmodel.h"
#include "namespacespage.h"
#include "ui_namespacespage.h"
#include "viewsetup.h"

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
NamespacesPage::NamespacesPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NamespacesPage)
    , _namespacesModel(new NamespacesModel(this))
{
    ui->setupUi(this);
    ViewSetup::splitByStretch(ui->splitter);
    ViewSetup::bindCopyButton(ui->copyIndexButton, ui->indexEdit);
    ViewSetup::bindCopyButton(ui->copyUriButton, ui->uriEdit);
    ui->importButton->setIcon(AppIcons::icon(QStringLiteral("download")));
    ui->exportButton->setIcon(AppIcons::icon(QStringLiteral("upload")));

    ViewSetup::configureTable(ui->namespacesView);
    _searchModel = ViewSetup::attachTableSearch(ui->namespacesView, _namespacesModel, ui->searchEdit);
    ViewSetup::fitColumnsWhenFilled(ui->namespacesView);

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
/// \brief Fills the details panel from the selected namespace.
/// \param current Selected table index.
///
void NamespacesPage::showNamespace(const QModelIndex &current)
{
    const QModelIndex source = _searchModel->mapToSource(current);
    ui->detailsHost->setEnabled(source.isValid());
    if (!source.isValid())
        return;
    const NamespaceInfo &ns = _namespacesModel->item(source.row());
    ui->indexEdit->setText(QString::number(ns.index));
    ui->uriEdit->setText(ns.uri);
    ui->versionEdit->setText(ns.version);
    ui->publicationDateEdit->setDate(ns.publicationDate);
    ui->descriptionEdit->setPlainText(ns.description);
}
