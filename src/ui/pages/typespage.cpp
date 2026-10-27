// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file typespage.cpp
/// \brief Implements the type system page.
///

#include <algorithm>

#include <QAbstractProxyModel>

#include "appicons.h"
#include "itemroles.h"
#include "referencesmodel.h"
#include "server/typecatalog.h"
#include "settingsstore.h"
#include "subtypesmodel.h"
#include "treemodel.h"
#include "typespage.h"
#include "ui_typespage.h"
#include "viewsetup.h"

namespace {

constexpr int kDescriptionLines = 2;
const char *const kNestedSubtypesKey = "types/nestedSubtypes";

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
    ui->nestedSubtypesButton->setIcon(AppIcons::icon(QStringLiteral("list-tree")));
    ui->nestedSubtypesButton->setChecked(SettingsStore().value(kNestedSubtypesKey, true).toBool());

    ViewSetup::installDelegate(ui->typesTree);
    ViewSetup::attachSearch(ui->typesTree, _treeModel, ui->searchEdit);
    ViewSetup::configureTable(ui->subtypesView);
    ui->subtypesView->setModel(_subtypesModel);
    ViewSetup::fitColumnsWhenFilled(ui->subtypesView);
    ViewSetup::configureTable(ui->referencesView);
    ui->referencesView->setModel(_referencesModel);
    ViewSetup::fitColumnsWhenFilled(ui->referencesView);
    ViewSetup::growColumnWithContents(ui->referencesView, ReferencesModel::ColReferenceType);

    connect(ui->typesTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TypesPage::showTreeNode);
    connect(ui->subtypesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TypesPage::showSubtype);
    connect(ui->allTypesCheck, &QCheckBox::toggled, this, &TypesPage::showTree);
    connect(ui->nestedSubtypesButton, &QToolButton::toggled, this, &TypesPage::setNestedSubtypesShown);
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
    QHash<QString, QString> supertypes;
    const QList<TypeInfo> subtypes = subtypesOf(current, supertypes);
    QHash<QString, QString> iconNames;
    for (const TypeInfo &subtype : subtypes)
        iconNames.insert(subtype.nodeId, TypeCatalog::typeIconName(_types, subtype.nodeId));
    _subtypesModel->setSubtypes(subtypes, supertypes, iconNames);
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
/// \param current Selected subtype index.
///
void TypesPage::showSubtype(const QModelIndex &current)
{
    if (current.isValid())
        showProperties(current.data(ItemRoles::Id).toString());
}

///
/// \brief Lists the subtypes of the selected type again with or without their own subtypes, keeps the
///        selected subtype when it is still listed and remembers the choice.
/// \param shown Whether subtypes of subtypes are listed.
///
void TypesPage::setNestedSubtypesShown(bool shown)
{
    SettingsStore().setValue(kNestedSubtypesKey, shown);
    const QString subtypeId = ViewSetup::currentId(ui->subtypesView);
    showTreeNode(ui->typesTree->currentIndex());
    if (!subtypeId.isEmpty())
        ViewSetup::selectById(ui->subtypesView, subtypeId);
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
/// \brief Collects the subtypes a tree node shows, at every depth while nested subtypes are shown, so the
///        list follows the standard types filter but not the search.
/// \param node Tree index of a type or of the Types folder; invalid collects none.
/// \param supertypes Receives the supertype node ids keyed by the node id of each nested subtype.
/// \return Subtypes depth-first, siblings ordered by browse name.
///
QList<TypeInfo> TypesPage::subtypesOf(const QModelIndex &node, QHash<QString, QString> &supertypes) const
{
    const auto *proxy = qobject_cast<const QAbstractProxyModel *>(node.model());
    const QModelIndex source = proxy ? proxy->mapToSource(node) : node;
    QList<TypeInfo> subtypes;
    if (source.isValid())
        appendSubtypes(source, {}, subtypes, supertypes);
    return subtypes;
}

///
/// \brief Appends the subtypes below a node of the type tree, each followed by its own subtypes while nested
///        subtypes are shown.
/// \param source Node of the unfiltered type tree.
/// \param supertypeId Node id recorded as the supertype of the appended types; empty keeps them top-level.
/// \param subtypes Receives the subtypes.
/// \param supertypes Receives the supertype of each nested subtype.
///
void TypesPage::appendSubtypes(const QModelIndex &source, const QString &supertypeId, QList<TypeInfo> &subtypes,
                               QHash<QString, QString> &supertypes) const
{
    QList<QModelIndex> children;
    for (int row = 0; row < _treeModel->rowCount(source); ++row)
        children.append(_treeModel->index(row, 0, source));
    std::sort(children.begin(), children.end(), [this](const QModelIndex &a, const QModelIndex &b) {
        return _types.value(a.data(ItemRoles::Id).toString()).browseName
             < _types.value(b.data(ItemRoles::Id).toString()).browseName;
    });
    for (const QModelIndex &child : std::as_const(children)) {
        const QString typeId = child.data(ItemRoles::Id).toString();
        subtypes.append(_types.value(typeId));
        if (!supertypeId.isEmpty())
            supertypes.insert(typeId, supertypeId);
        if (ui->nestedSubtypesButton->isChecked())
            appendSubtypes(child, typeId, subtypes, supertypes);
    }
}
