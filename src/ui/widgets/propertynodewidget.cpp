// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file propertynodewidget.cpp
/// \brief Implements the editor of a property of a variable.
///

#include <QDoubleValidator>

#include "propertynodewidget.h"
#include "ui_propertynodewidget.h"
#include "viewsetup.h"

namespace {

const QString kRange = QStringLiteral("Range");
const QString kRangeSeparator = QStringLiteral("; ");

///
/// \brief Builds a validator of a Range limit; the C locale keeps the decimal point of the stored text.
/// \param parent Owner of the validator.
/// \return Validator of doubles.
///
QDoubleValidator *limitValidator(QObject *parent)
{
    auto *validator = new QDoubleValidator(parent);
    validator->setLocale(QLocale::c());
    return validator;
}

}

///
/// \brief Builds the editor.
/// \param parent Parent widget.
///
PropertyNodeWidget::PropertyNodeWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PropertyNodeWidget)
{
    ui->setupUi(this);
    ViewSetup::bindCopyButton(ui->propertyCopyNodeIdButton, ui->propertyNodeIdEdit);
    connect(ui->propertyApplyButton, &QPushButton::clicked, this, &PropertyNodeWidget::requestApply);
    connect(ui->propertyRevertButton, &QPushButton::clicked, this, &PropertyNodeWidget::revert);
    ui->propertyLowEdit->setValidator(limitValidator(this));
    ui->propertyHighEdit->setValidator(limitValidator(this));
    ViewSetup::enableOnEdit(ui->propertyNodeHost, ui->propertyApplyButton);
    clear();
}

///
/// \brief Destroys the editor.
///
PropertyNodeWidget::~PropertyNodeWidget()
{
    delete ui;
}

///
/// \brief Shows a property; its value text is the initial value, editable unless the property is read-only, and a
///        Range is edited as its Low and High limits.
/// \param property Property as listed.
///
void PropertyNodeWidget::setNode(const VariableInfo &property)
{
    _property = property;
    const bool range = property.shownDataType() == kRange;
    ui->propertyValueLabel->setVisible(!range);
    ui->propertyValueEdit->setVisible(!range);
    const QList<QWidget *> limitFields = { ui->propertyLowLabel, ui->propertyLowEdit, ui->propertyHighLabel,
                                           ui->propertyHighEdit };
    for (QWidget *widget : limitFields)
        widget->setVisible(range);
    const QStringList limits = property.initialValue.toString().split(QLatin1Char(';'));
    ui->propertyLowEdit->setText(limits.value(0).trimmed());
    ui->propertyHighEdit->setText(limits.value(1).trimmed());
    ui->propertyLowEdit->setReadOnly(property.readOnly);
    ui->propertyHighEdit->setReadOnly(property.readOnly);
    ui->propertyNodeIdEdit->setText(property.nodeId);
    ui->propertyTypeEdit->setText(property.typeName);
    ui->propertyNameEdit->setText(property.displayName);
    ui->propertyDataTypeEdit->setText(property.shownDataType());
    ui->propertyAccessEdit->setText(property.access);
    ui->propertyValueEdit->setText(property.initialValue.toString());
    ui->propertyValueEdit->setReadOnly(property.readOnly);
    ui->propertyNodeActions->setEnabled(!property.readOnly);
    ui->propertyApplyButton->setEnabled(false);
}

///
/// \brief Empties and disables the editor.
///
void PropertyNodeWidget::clear()
{
    setNode(VariableInfo());
    ui->propertyNodeActions->setEnabled(false);
}

///
/// \brief Retranslates the editor when the language changes.
/// \param event Change event.
///
void PropertyNodeWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QWidget::changeEvent(event);
}

///
/// \brief Asks to apply the typed value of the shown property.
///
void PropertyNodeWidget::requestApply()
{
    if (_property.nodeId.isEmpty())
        return;
    VariableInfo edited = _property;
    edited.initialValue = typedValue();
    emit applyRequested(edited);
}

///
/// \brief Reads the typed value.
/// \return Value text, "Low; High" for a Range, or invalid when nothing was edited.
///
QVariant PropertyNodeWidget::typedValue() const
{
    if (_property.shownDataType() != kRange)
        return ui->propertyValueEdit->isModified() ? QVariant(ui->propertyValueEdit->text()) : QVariant();
    if (!ui->propertyLowEdit->isModified() && !ui->propertyHighEdit->isModified())
        return {};
    return ui->propertyLowEdit->text().trimmed() + kRangeSeparator + ui->propertyHighEdit->text().trimmed();
}

///
/// \brief Discards the typed value by showing the property again.
///
void PropertyNodeWidget::revert()
{
    setNode(_property);
}
