// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typespage.cpp
/// \brief Implements the type system page.
///

#include <algorithm>

#include <QAbstractProxyModel>

#include "itemroles.h"
#include "referencesmodel.h"
#include "server/typecatalog.h"
#include "subtypesmodel.h"
#include "treemodel.h"
#include "typespage.h"
#include "ui_typespage.h"
#include "viewsetup.h"

namespace {

constexpr int kDescriptionLines = 2;

}

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
    ViewSetup::fitHeightToLines(ui->descriptionEdit, kDescriptionLines);
    ViewSetup::splitByStretch(ui->splitter);
    ViewSetup::bindCopyButton(ui->copyBrowseNameButton, ui->browseNameEdit);
    ViewSetup::bindCopyButton(ui->copyNodeIdButton, ui->nodeIdEdit);

    ViewSetup::installDelegate(ui->typesTree);
    ViewSetup::attachSearch(ui->typesTree, _treeModel, ui->searchEdit);
    ViewSetup::configureTable(ui->subtypesView);
    ui->subtypesView->setModel(_subtypesModel);
    ViewSetup::fitColumnsWhenFilled(ui->subtypesView);
    ViewSetup::configureTable(ui->referencesView);
    ui->referencesView->setModel(_referencesModel);
    ViewSetup::fitColumnsWhenFilled(ui->referencesView);

    connect(ui->typesTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TypesPage::showTreeNode);
    connect(ui->subtypesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TypesPage::showSubtype);
    connect(ui->allTypesCheck, &QCheckBox::toggled, this, &TypesPage::showTree);
}

///
/// \brief Destroys the page.
///
TypesPage::~TypesPage()
{
    delete ui;
}

///
/// \brief Shows a type system; the tree holds every type or the relevant ones, as the page's check box says.
/// \param types Types keyed by node id.
///
void TypesPage::setTypes(const QHash<QString, TypeInfo> &types)
{
    _types = types;
    showTree();
}

///
/// \brief Fills the tree with every type or only the relevant ones and selects the initial type.
///
void TypesPage::showTree()
{
    _treeModel->setNodes(TypeCatalog::tree(_types, ui->allTypesCheck->isChecked()));
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
/// \brief Lists the subtypes of the selected type, or the root types for the Types folder, and shows
///        the type's properties.
/// \param current Selected tree index.
///
void TypesPage::showTreeNode(const QModelIndex &current)
{
    const QString typeId = current.data(ItemRoles::Id).toString();
    _subtypesModel->setItems(subtypesOf(current));
    _shownTypeName = typeId.isEmpty() ? current.data().toString() : _types.value(typeId).browseName;
    showSubtypesTitle();
    showProperties(typeId);
}

///
/// \brief Titles the subtypes panel after the type selected in the tree.
///
void TypesPage::showSubtypesTitle()
{
    ui->subtypesTitle->setText(_shownTypeName.isEmpty() ? tr("Subtype Definitions")
                                                        : tr("Subtype Definitions (%1)").arg(_shownTypeName));
}

///
/// \brief Retranslates the page when the language changes, keeping the shown type.
/// \param event Change event.
///
void TypesPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        showSubtypesTitle();
        _subtypesModel->retranslate();
        _referencesModel->retranslate();
    }
    QWidget::changeEvent(event);
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
        ui->baseTypeEdit->clear();
        ui->isAbstractEdit->clear();
        ui->descriptionEdit->clear();
        _referencesModel->clear();
        return;
    }

    ui->browseNameEdit->setText(found->browseName);
    ui->nodeIdEdit->setText(found->nodeId);
    ui->baseTypeEdit->setText(_types.value(found->baseTypeId).browseName);
    ui->isAbstractEdit->setText(found->isAbstract ? QStringLiteral("true") : QStringLiteral("false"));
    ui->descriptionEdit->setPlainText(found->description);
    _referencesModel->setItems(found->references);
}

///
/// \brief Collects the subtypes a tree node shows, so the table follows the standard types filter
///        but not the search.
/// \param node Tree index of a type or of the Types folder; invalid collects none.
/// \return Subtypes ordered by browse name.
///
QList<TypeInfo> TypesPage::subtypesOf(const QModelIndex &node) const
{
    const auto *proxy = qobject_cast<const QAbstractProxyModel *>(node.model());
    const QModelIndex source = proxy ? proxy->mapToSource(node) : node;
    QList<TypeInfo> subtypes;
    if (!source.isValid())
        return subtypes;
    for (int row = 0; row < _treeModel->rowCount(source); ++row)
        subtypes.append(_types.value(_treeModel->index(row, 0, source).data(ItemRoles::Id).toString()));
    std::sort(subtypes.begin(), subtypes.end(), [](const TypeInfo &a, const TypeInfo &b) {
        return a.browseName < b.browseName;
    });
    return subtypes;
}
