// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacespage.cpp
/// \brief Implements the namespaces page.
///

#include <QFileInfo>
#include <QSortFilterProxyModel>

#include "appicons.h"
#include "namespacesmodel.h"
#include "namespacespage.h"
#include "ui_namespacespage.h"
#include "viewsetup.h"

namespace {

const QString kModelColumnSample = QStringLiteral("Opc.Ua.Di.NodeSet2.xml");

}

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
    ui->removeModelButton->setIcon(AppIcons::icon(QStringLiteral("trash")));
    ui->exportButton->setEnabled(false);
    ui->removeModelButton->setEnabled(false);

    ViewSetup::configureTable(ui->namespacesView);
    _searchModel = ViewSetup::attachTableSearch(ui->namespacesView, _namespacesModel, ui->searchEdit);
    ViewSetup::fitColumnsWhenFilled(ui->namespacesView);
    ViewSetup::reserveColumnWidth(ui->namespacesView, NamespacesModel::ColModel, kModelColumnSample);

    connect(ui->namespacesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &NamespacesPage::showNamespace);
    connect(ui->importButton, &QPushButton::clicked, this, &NamespacesPage::importRequested);
    connect(ui->exportButton, &QPushButton::clicked, this, &NamespacesPage::requestExport);
    connect(ui->removeModelButton, &QPushButton::clicked, this, &NamespacesPage::requestModelRemoval);
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
/// \brief Retranslates the page when the language changes.
/// \param event Change event.
///
void NamespacesPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        _namespacesModel->retranslate();
    }
    QWidget::changeEvent(event);
}

///
/// \brief Fills the details panel from the selected namespace.
///
void NamespacesPage::showNamespace()
{
    const NamespaceInfo *ns = currentNamespace();
    ui->detailsHost->setEnabled(ns != nullptr);
    ui->exportButton->setEnabled(ns && ns->index > 0);
    ui->removeModelButton->setEnabled(ns && !ns->modelFile.isEmpty());
    if (!ns)
        return;
    ui->indexEdit->setText(QString::number(ns->index));
    ui->uriEdit->setText(ns->uri);
    ui->modelEdit->setText(QFileInfo(ns->modelFile).fileName());
    ui->modelEdit->setToolTip(ns->modelFile);
    ui->versionEdit->setText(ns->version);
    ui->publicationDateEdit->setDate(ns->publicationDate);
    ui->descriptionEdit->setPlainText(ns->description);
}

///
/// \brief Returns the selected namespace.
/// \return Namespace, or null when nothing is selected.
///
const NamespaceInfo *NamespacesPage::currentNamespace() const
{
    const QModelIndex source = _searchModel->mapToSource(ui->namespacesView->currentIndex());
    return source.isValid() ? &_namespacesModel->item(source.row()) : nullptr;
}

///
/// \brief Asks to save the nodes of the selected namespace unless it is the standard namespace 0.
///
void NamespacesPage::requestExport()
{
    const NamespaceInfo *ns = currentNamespace();
    if (ns && ns->index > 0)
        emit exportRequested(ns->uri);
}

///
/// \brief Asks to unload the imported model the selected namespace came from.
///
void NamespacesPage::requestModelRemoval()
{
    const NamespaceInfo *ns = currentNamespace();
    if (ns && !ns->modelFile.isEmpty())
        emit modelRemovalRequested(ns->modelFile);
}
