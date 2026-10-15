// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typespage.cpp
/// \brief Implements the type system page.
///

#include <algorithm>

#include "itemroles.h"
#include "referencesmodel.h"
#include "subtypesmodel.h"
#include "treemodel.h"
#include "typespage.h"
#include "ui_typespage.h"
#include "viewsetup.h"

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
TypesPage::TypesPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TypesPage)
    , _treeModel(new TreeModel(this))
    , _subtypesModel(new SubtypesModel(this))
    , _referencesModel(new ReferencesModel(this))
{
    ui->setupUi(this);
    ViewSetup::splitByStretch(ui->splitter);
    ViewSetup::bindCopyButton(ui->copyBrowseNameButton, ui->browseNameEdit);
    ViewSetup::bindCopyButton(ui->copyNodeIdButton, ui->nodeIdEdit);

    ViewSetup::installDelegate(ui->typesTree);
    ViewSetup::attachSearch(ui->typesTree, _treeModel, ui->searchEdit);
    ViewSetup::configureTable(ui->subtypesView);
    ui->subtypesView->setModel(_subtypesModel);
    ViewSetup::configureTable(ui->referencesView);
    ui->referencesView->setModel(_referencesModel);

    connect(ui->typesTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TypesPage::showTreeNode);
    connect(ui->subtypesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TypesPage::showSubtype);
}

///
/// \brief Destroys the page.
///
TypesPage::~TypesPage()
{
    delete ui;
}

///
/// \brief Shows a type system.
/// \param typeTree Type hierarchy tree.
/// \param types Types keyed by node id.
///
void TypesPage::setTypes(const QList<TreeNode> &typeTree, const QHash<QString, TypeInfo> &types)
{
    _types = types;

    QStringList names;
    for (const TypeInfo &type : types)
        names.append(type.browseName);
    names.sort();
    ui->baseTypeCombo->clear();
    ui->baseTypeCombo->addItems(names);

    _treeModel->setNodes(typeTree);
    ViewSetup::applyInitialState(ui->typesTree);
}

///
/// \brief Returns the subtypes table model.
/// \return Model of the subtypes of the selected type.
///
const SubtypesModel *TypesPage::subtypesModel() const
{
    return _subtypesModel;
}

///
/// \brief Lists the subtypes of the selected type and shows its properties.
/// \param current Selected tree index.
///
void TypesPage::showTreeNode(const QModelIndex &current)
{
    const QString typeId = current.data(ItemRoles::Id).toString();
    const TypeInfo type = _types.value(typeId);
    _subtypesModel->setItems(subtypesOf(typeId));
    ui->subtypesTitle->setText(type.browseName.isEmpty()
                                   ? tr("Subtype Definitions")
                                   : tr("Subtype Definitions (%1)").arg(type.browseName));
    showProperties(typeId);
}

///
/// \brief Shows the properties of the selected subtype.
/// \param current Selected table index.
///
void TypesPage::showSubtype(const QModelIndex &current)
{
    if (current.isValid())
        showProperties(_subtypesModel->item(current.row()).nodeId);
}

///
/// \brief Fills the properties panel and the reference counts from a type.
/// \param typeId Type node id; unknown ids clear and disable the panel.
///
void TypesPage::showProperties(const QString &typeId)
{
    const auto found = _types.constFind(typeId);
    const bool known = found != _types.cend();
    ui->propertiesHost->setEnabled(known);
    if (!known) {
        ui->browseNameEdit->clear();
        ui->nodeIdEdit->clear();
        ui->descriptionEdit->clear();
        _referencesModel->clear();
        return;
    }

    ui->browseNameEdit->setText(found->browseName);
    ui->nodeIdEdit->setText(found->nodeId);
    ui->baseTypeCombo->setCurrentText(_types.value(found->baseTypeId).browseName);
    ui->isAbstractCombo->setCurrentIndex(found->isAbstract ? 1 : 0);
    ui->descriptionEdit->setPlainText(found->description);
    _referencesModel->setItems(found->references);
}

///
/// \brief Collects the direct subtypes of a type.
/// \param typeId Supertype node id.
/// \return Subtypes ordered by browse name.
///
QList<TypeInfo> TypesPage::subtypesOf(const QString &typeId) const
{
    QList<TypeInfo> subtypes;
    for (const TypeInfo &type : _types) {
        if (!typeId.isEmpty() && type.baseTypeId == typeId)
            subtypes.append(type);
    }
    std::sort(subtypes.begin(), subtypes.end(), [](const TypeInfo &a, const TypeInfo &b) {
        return a.browseName < b.browseName;
    });
    return subtypes;
}
