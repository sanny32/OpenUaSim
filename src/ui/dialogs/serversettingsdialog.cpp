// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file serversettingsdialog.cpp
/// \brief Implements the dialog that edits the server description.
///

#include <QPushButton>

#include "project/serversettingsrules.h"
#include "serversettingsdialog.h"
#include "ui_serversettingsdialog.h"

///
/// \brief Builds the dialog showing the current settings.
/// \param settings Settings to edit.
/// \param parent Parent widget.
///
ServerSettingsDialog::ServerSettingsDialog(const ServerSettings &settings, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ServerSettingsDialog)
{
    ui->setupUi(this);
    ui->maxSessionsSpin->setMaximum(kMaxSessionLimit);
    ui->historyMaxValuesSpin->setMaximum(kMaxHistoryValues);
    ui->historyMaxAgeSpin->setMaximum(kMaxHistoryAgeMinutes);
    ui->discoveryIntervalSpin->setRange(kMinDiscoveryIntervalSeconds, kMaxDiscoveryIntervalSeconds);
    ui->applicationNameEdit->setText(settings.applicationName);
    ui->applicationUriEdit->setText(settings.applicationUri);
    ui->maxSessionsSpin->setValue(settings.maxSessions);
    ui->historyMaxValuesSpin->setValue(settings.history.maxValues);
    ui->historyMaxAgeSpin->setValue(settings.history.maxAgeMinutes);
    ui->discoveryCheck->setChecked(settings.discovery.enabled);
    ui->discoveryUrlEdit->setText(settings.discovery.url);
    ui->discoveryIntervalSpin->setValue(settings.discovery.intervalSeconds);

    connect(ui->applicationNameEdit, &QLineEdit::textChanged, this, &ServerSettingsDialog::validate);
    connect(ui->applicationUriEdit, &QLineEdit::textChanged, this, &ServerSettingsDialog::validate);
    connect(ui->discoveryCheck, &QCheckBox::toggled, this, &ServerSettingsDialog::validate);
    connect(ui->discoveryUrlEdit, &QLineEdit::textChanged, this, &ServerSettingsDialog::validate);
    validate();
}

///
/// \brief Destroys the dialog.
///
ServerSettingsDialog::~ServerSettingsDialog()
{
    delete ui;
}

///
/// \brief Returns the edited settings.
/// \return Settings with the name, URI and discovery server URL trimmed.
///
ServerSettings ServerSettingsDialog::settings() const
{
    ServerSettings settings;
    settings.applicationName = ui->applicationNameEdit->text().trimmed();
    settings.applicationUri = ui->applicationUriEdit->text().trimmed();
    settings.maxSessions = ui->maxSessionsSpin->value();
    settings.history = { ui->historyMaxValuesSpin->value(), ui->historyMaxAgeSpin->value() };
    settings.discovery = { ui->discoveryCheck->isChecked(), ui->discoveryUrlEdit->text().trimmed(),
                           ui->discoveryIntervalSpin->value() };
    return settings;
}

///
/// \brief Enables the discovery fields while registration is on, shows why the entered settings cannot be applied
///        and allows OK only when they can.
///
void ServerSettingsDialog::validate()
{
    const bool discovery = ui->discoveryCheck->isChecked();
    ui->discoveryUrlEdit->setEnabled(discovery);
    ui->discoveryIntervalSpin->setEnabled(discovery);
    const QString problem = ServerSettingsRules::problem(settings());
    ui->problemLabel->setText(problem);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(problem.isEmpty());
}
