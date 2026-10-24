// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addnodedialog.cpp
/// \brief Implements the dialog that describes a new folder, object, variable or method.
///

#include <QPushButton>
#include <QRegularExpression>

#include "addnodedialog.h"
#include "appicons.h"
#include "project/portablenodeid.h"
#include "project/projectnodes.h"
#include "server/typecatalog.h"
#include "simulation/valuecast.h"
#include "ui_addnodedialog.h"
#include "viewsetup.h"

namespace {

///
/// \brief Tells whether a text is a node id in XML notation.
/// \param nodeId Text to check.
/// \return True for forms such as "ns=1;s=Name" or "i=5000".
///
bool isNodeIdNotation(const QString &nodeId)
{
    static const QRegularExpression pattern(QStringLiteral("^(ns=\\d+;)?[isgb]=.+$"));
    return pattern.match(nodeId).hasMatch();
}

}

///
/// \brief Builds the dialog.
/// \param nodes Project node tree whose folders and objects are offered as parents.
/// \param parentId Folder or object selected first; empty or unknown selects the top level.
/// \param parent Parent widget.
///
AddNodeDialog::AddNodeDialog(const QList<NodeDefinition> &nodes, const QString &parentId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddNodeDialog)
    , _nodes(nodes)
    , _usedIds(ProjectNodes::nodeIds(nodes))
{
    ui->setupUi(this);
    ViewSetup::configureAccessCombo(ui->accessCombo);
    const QString objects = tr("Objects");
    ui->parentCombo->addItem(AppIcons::icon(QStringLiteral("folder")), objects, QString());
    fillParentCombo(nodes, objects);
    ui->parentCombo->setCurrentIndex(qMax(0, ui->parentCombo->findData(parentId)));
    ui->dataTypeCombo->addItems(ValueCast::dataTypes());
    ui->dataTypeCombo->setCurrentText(QStringLiteral("Double"));
    ViewSetup::fillVariableIconCombo(ui->iconCombo);

    connect(ui->parentCombo, &QComboBox::currentIndexChanged, this, &AddNodeDialog::validate);
    connect(ui->kindCombo, &QComboBox::currentIndexChanged, this, &AddNodeDialog::updateKindFields);
    connect(ui->browseNameEdit, &QLineEdit::textChanged, this, &AddNodeDialog::validate);
    connect(ui->nodeIdEdit, &QLineEdit::textChanged, this, &AddNodeDialog::validate);
    updateKindFields();
}

///
/// \brief Destroys the dialog.
///
AddNodeDialog::~AddNodeDialog()
{
    delete ui;
}

///
/// \brief Offers the types of the server for the new node.
/// \param types Types keyed by server node id, as TypeCatalog::types() returns them.
/// \param namespaces Namespace URIs of the server; the position of a URI is its index.
///
void AddNodeDialog::setTypes(const QHash<QString, TypeInfo> &types, const QStringList &namespaces)
{
    _types = types;
    _namespaces = namespaces;
    fillTypeCombo();
}

///
/// \brief Returns the chosen parent.
/// \return Folder or object id; empty for the top level.
///
QString AddNodeDialog::parentId() const
{
    return ui->parentCombo->currentData().toString();
}

///
/// \brief Returns the described node.
/// \return Node without children; a variable starts with the default value of its type and no simulation.
///
NodeDefinition AddNodeDialog::node() const
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind(ui->kindCombo->currentIndex());
    node.browseName = ui->browseNameEdit->text().trimmed();
    node.nodeId = ui->nodeIdEdit->text().trimmed();
    if (node.kind != NodeDefinition::Kind::Method)
        node.typeDefinition = ui->typeCombo->currentData().toString();
    if (node.kind == NodeDefinition::Kind::Variable) {
        node.dataType = ui->dataTypeCombo->currentText();
        node.access = ui->accessCombo->currentData().toString();
        node.icon = ui->iconCombo->currentData().toString();
        node.initialValue = ValueCast::defaultValue(node.dataType);
    } else if (node.kind == NodeDefinition::Kind::Method) {
        node.method = NodeDefinition::MethodAction(ui->actionCombo->currentIndex());
    }
    return node;
}

///
/// \brief Returns why the entered node cannot be added.
/// \return Problem text, or empty when OK may be pressed.
///
QString AddNodeDialog::problem() const
{
    const QString name = ui->browseNameEdit->text().trimmed();
    const QString nodeId = ui->nodeIdEdit->text().trimmed();
    if (name.isEmpty())
        return tr("Enter a browse name.");
    if (NodeDefinition::Kind(ui->kindCombo->currentIndex()) == NodeDefinition::Kind::Method && parentId().isEmpty())
        return tr("Choose a folder or object for the method.");
    if (ProjectNodes::childNames(_nodes, parentId()).contains(name))
        return tr("A sibling is already named '%1'.").arg(name);
    if (!nodeId.isEmpty() && !isNodeIdNotation(nodeId))
        return tr("'%1' is not a node id such as ns=1;s=Name.").arg(nodeId);
    if (_usedIds.contains(nodeId))
        return tr("The node id '%1' is already used.").arg(nodeId);
    return {};
}

///
/// \brief Adds the folders and objects of a tree to the parent combo, labelled by their browse path.
/// \param nodes Nodes of one tree level.
/// \param path Browse path of the level; paths tell apart equally named containers on different branches.
///
void AddNodeDialog::fillParentCombo(const QList<NodeDefinition> &nodes, const QString &path)
{
    for (const NodeDefinition &node : nodes) {
        if (node.declared || (node.kind != NodeDefinition::Kind::Folder && node.kind != NodeDefinition::Kind::Object))
            continue;
        const QString nodePath = path + QStringLiteral(" / ") + node.browseName;
        ui->parentCombo->addItem(AppIcons::icon(QStringLiteral("folder")), nodePath, node.nodeId);
        fillParentCombo(node.children, nodePath);
    }
}

///
/// \brief Lists the types of the chosen kind; the first, standard one leaves the type definition empty.
///
void AddNodeDialog::fillTypeCombo()
{
    ui->typeCombo->clear();
    const QList<TypeInfo> types = TypeCatalog::instanceTypes(_types, NodeDefinition::Kind(ui->kindCombo->currentIndex()));
    for (qsizetype index = 0; index < types.size(); ++index) {
        const TypeInfo &type = types.at(index);
        ui->typeCombo->addItem(AppIcons::icon(TypeCatalog::iconName(_types, type.nodeId)), type.browseName,
                               index == 0 ? QString() : PortableNodeId::fromServer(type.nodeId, _namespaces));
    }
    ui->typeCombo->setEnabled(types.size() > 1);
}

///
/// \brief Enables the data type, access and icon fields for variables and the action for methods only.
///
void AddNodeDialog::updateKindFields()
{
    fillTypeCombo();
    const auto kind = NodeDefinition::Kind(ui->kindCombo->currentIndex());
    const bool variable = kind == NodeDefinition::Kind::Variable;
    ui->dataTypeCombo->setEnabled(variable);
    ui->accessCombo->setEnabled(variable);
    ui->iconCombo->setEnabled(variable);
    ui->actionCombo->setEnabled(kind == NodeDefinition::Kind::Method);
    validate();
}

///
/// \brief Shows the current problem and enables OK only without one.
///
void AddNodeDialog::validate()
{
    const QString text = problem();
    ui->problemLabel->setText(text);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(text.isEmpty());
}
