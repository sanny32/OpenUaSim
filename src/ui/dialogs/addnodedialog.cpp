// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addnodedialog.cpp
/// \brief Implements the dialog that describes a new folder, object, variable or method.
///

#include <QRegularExpression>
#include <QSignalBlocker>

#include "addnodedialog.h"
#include "appicons.h"
#include "messagedialog.h"
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
    ui->arrayLengthSpin->setMaximum(kMaxArrayLength);

    connect(ui->kindCombo, &QComboBox::currentIndexChanged, this, &AddNodeDialog::updateKindFields);
    connect(ui->typeCombo, &QComboBox::currentIndexChanged, this, &AddNodeDialog::updateTypeFields);
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
    updateTypeFields();
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
        node.arrayLength = ui->arrayLengthSpin->value();
        node.access = ui->accessCombo->currentData().toString();
        node.icon = ui->iconCombo->currentData().toString();
        const QVariant value = ValueCast::defaultValue(node.dataType);
        node.initialValue = node.arrayLength > 0
            ? QVariant(ValueCast::toArray(value, node.dataType, node.arrayLength)) : value;
    } else if (node.kind == NodeDefinition::Kind::Method) {
        node.method = NodeDefinition::MethodAction(ui->actionCombo->currentIndex());
    }
    return node;
}

///
/// \brief Returns why the entered node cannot be added.
/// \return Problem text, or empty when the node can be added.
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
    if (NodeDefinition::Kind(ui->kindCombo->currentIndex()) == NodeDefinition::Kind::Variable) {
        const TypeInfo type = _types.value(ui->typeCombo->currentData(Qt::UserRole + 1).toString());
        if (!type.dataTypeId.isEmpty() || !ui->typeCombo->currentData().toString().isEmpty())
            return TypeCatalog::variableProblem(type, ui->dataTypeCombo->currentText(), ui->arrayLengthSpin->value());
    }
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
    const QSignalBlocker blocker(ui->typeCombo);
    ui->typeCombo->clear();
    const QList<TypeInfo> types = TypeCatalog::instanceTypes(_types, NodeDefinition::Kind(ui->kindCombo->currentIndex()));
    for (qsizetype index = 0; index < types.size(); ++index) {
        const TypeInfo &type = types.at(index);
        ui->typeCombo->addItem(AppIcons::icon(TypeCatalog::iconName(_types, type.nodeId)), type.browseName,
                               index == 0 ? QString() : PortableNodeId::fromServer(type.nodeId, _namespaces));
        ui->typeCombo->setItemData(int(index), type.nodeId, Qt::UserRole + 1);
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
    ui->accessCombo->setEnabled(variable);
    ui->iconCombo->setEnabled(variable);
    ui->actionCombo->setEnabled(kind == NodeDefinition::Kind::Method);
    updateTypeFields();
}

///
/// \brief Offers writable data types and array lengths permitted by the selected variable type.
///
void AddNodeDialog::updateTypeFields()
{
    const bool variable = NodeDefinition::Kind(ui->kindCombo->currentIndex()) == NodeDefinition::Kind::Variable;
    const TypeInfo type = _types.value(ui->typeCombo->currentData(Qt::UserRole + 1).toString());
    const QStringList dataTypes = variable && !type.dataTypeId.isEmpty()
        ? type.valueDataTypes : ValueCast::dataTypes();
    const QString previous = ui->dataTypeCombo->currentText();
    const QSignalBlocker dataTypeBlocker(ui->dataTypeCombo);
    const QSignalBlocker lengthBlocker(ui->arrayLengthSpin);
    ui->dataTypeCombo->clear();
    ui->dataTypeCombo->addItems(dataTypes);
    ui->dataTypeCombo->setCurrentIndex(qMax(0, ui->dataTypeCombo->findText(previous)));
    ui->dataTypeCombo->setEnabled(variable && !dataTypes.isEmpty());
    int minimum = 0;
    int maximum = kMaxArrayLength;
    if (variable && !type.dataTypeId.isEmpty()) {
        if (type.valueRank == -1)
            maximum = 0;
        else if (type.valueRank >= 0)
            minimum = 1;
        if (type.arrayDimensions.size() == 1 && type.arrayDimensions.first() > 0
            && type.arrayDimensions.first() <= quint32(kMaxArrayLength))
            minimum = maximum = int(type.arrayDimensions.first());
    }
    ui->arrayLengthSpin->setRange(minimum, maximum);
    ui->arrayLengthSpin->setSpecialValueText(minimum == 0 ? tr("Scalar") : QString());
    ui->arrayLengthSpin->setEnabled(variable && minimum != maximum && type.valueRank <= 1);
}

///
/// \brief Closes the dialog, or keeps it open and shows a warning when the entered node cannot be added.
///
void AddNodeDialog::accept()
{
    const QString text = problem();
    if (!text.isEmpty()) {
        MessageDialog::warning(tr("Node not added"), text, this);
        return;
    }
    QDialog::accept();
}
