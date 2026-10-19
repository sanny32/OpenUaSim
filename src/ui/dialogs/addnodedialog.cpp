// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addnodedialog.cpp
/// \brief Implements the dialog that describes a new folder, object or variable.
///

#include <QPushButton>
#include <QRegularExpression>

#include "addnodedialog.h"
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
/// \param parentName Display name of the node that receives the new node.
/// \param siblingNames Browse names already used below that node.
/// \param usedIds Node ids already used in the project.
/// \param parent Parent widget.
///
AddNodeDialog::AddNodeDialog(const QString &parentName, const QStringList &siblingNames,
                             const QSet<QString> &usedIds, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddNodeDialog)
    , _siblingNames(siblingNames)
    , _usedIds(usedIds)
{
    ui->setupUi(this);
    ui->parentValue->setText(parentName);
    ui->dataTypeCombo->addItems(ValueCast::dataTypes());
    ui->dataTypeCombo->setCurrentText(QStringLiteral("Double"));
    ViewSetup::fillVariableIconCombo(ui->iconCombo);

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
/// \brief Returns the described node.
/// \return Node without children; a variable starts with the default value of its type and no simulation.
///
NodeDefinition AddNodeDialog::node() const
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind(ui->kindCombo->currentIndex());
    node.browseName = ui->browseNameEdit->text().trimmed();
    node.nodeId = ui->nodeIdEdit->text().trimmed();
    if (node.kind == NodeDefinition::Kind::Variable) {
        node.dataType = ui->dataTypeCombo->currentText();
        node.access = ui->accessCombo->currentText();
        node.icon = ui->iconCombo->currentData().toString();
        node.initialValue = ValueCast::defaultValue(node.dataType);
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
    if (_siblingNames.contains(name))
        return tr("A sibling is already named '%1'.").arg(name);
    if (!nodeId.isEmpty() && !isNodeIdNotation(nodeId))
        return tr("'%1' is not a node id such as ns=1;s=Name.").arg(nodeId);
    if (_usedIds.contains(nodeId))
        return tr("The node id '%1' is already used.").arg(nodeId);
    return {};
}

///
/// \brief Enables the data type, access and icon fields for variables only.
///
void AddNodeDialog::updateKindFields()
{
    const bool variable = NodeDefinition::Kind(ui->kindCombo->currentIndex()) == NodeDefinition::Kind::Variable;
    ui->dataTypeCombo->setEnabled(variable);
    ui->accessCombo->setEnabled(variable);
    ui->iconCombo->setEnabled(variable);
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
