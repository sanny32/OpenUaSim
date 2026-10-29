// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file opcuainfodialog.cpp
/// \brief Implements the OPC UA information dialog.
///

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <open62541/config.h>
#include <QtGlobal>

#include "apppalette.h"
#include "dependency_versions.h"
#include "server/listenersettings.h"
#include "appicons.h"
#include "opcuainfodialog.h"
#include "ui_opcuainfodialog.h"

///
/// \brief Builds the OPC UA information dialog and fills its runtime fields.
/// \param parent Parent widget.
///
OpcUaInfoDialog::OpcUaInfoDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::OpcUaInfoDialog)
{
    ui->setupUi(this);

    setupLogo();
    setupContent();
    setupFonts();
    setupLinks();
    setupLayout();

    ui->copyButton->setIcon(AppIcons::icon(QStringLiteral("copy")));

    connect(ui->copyButton, &QPushButton::clicked, this, &OpcUaInfoDialog::copyInfo);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

///
/// \brief Destroys the dialog and its generated UI.
///
OpcUaInfoDialog::~OpcUaInfoDialog()
{
    delete ui;
}

///
/// \brief Returns the plain-text diagnostic summary copied by the dialog.
/// \return Current OPC UA information summary.
///
QString OpcUaInfoDialog::copyText() const
{
    return QStringLiteral(
        "OPC UA Info\n\n"
        "Stack\n"
        "OPC UA SDK: %1\n"
        "SDK Version: %2\n"
        "Qt Version: %3\n"
        "QUaServer Version: %9\n\n"
        "Supported Security\n"
        "Security Policies: %4\n"
        "Message Security Modes: %5\n"
        "User Token Types: %6\n\n"
        "Protocol\n"
        "Transport: %7\n"
        "Application role: %8\n\n"
        "Resources\n"
        "OPC Foundation: https://opcfoundation.org\n"
        "OPC UA Specification: https://reference.opcfoundation.org")
        .arg(ui->sdkValue->text(),
             ui->sdkVersionValue->text(),
             ui->qtVersionValue->text(),
             ui->securityPoliciesValue->text(),
             ui->securityModesValue->text(),
             ui->userTokenTypesValue->text(),
             ui->transportValue->text(),
             ui->roleValue->text(),
             ui->quaServerVersionValue->text());
}

///
/// \brief Refreshes theme assets, translated values and caption alignment.
/// \param event Change event being handled.
///
void OpcUaInfoDialog::changeEvent(QEvent *event)
{
    AppBaseDialog::changeEvent(event);

    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        setupLogo();
    } else if (event->type() == QEvent::FontChange) {
        setupLayout();
    } else if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setupContent();
        setupLayout();
    }
}

///
/// \brief Copies the displayed OPC UA information to the clipboard.
///
void OpcUaInfoDialog::copyInfo()
{
    QApplication::clipboard()->setText(copyText());
}

///
/// \brief Populates the static and runtime labels.
///
void OpcUaInfoDialog::setupContent()
{
    QStringList policies { QStringLiteral("None") };
    QStringList modes { QStringLiteral("None") };
    QStringList tokens { QStringLiteral("Anonymous"), QStringLiteral("Username") };
#ifdef OUASIM_HAVE_ENCRYPTION
    policies = ListenerSettingsBuilder::securityPolicyNames();
    modes = ListenerSettingsBuilder::securityModeNames();
    tokens.append(QStringLiteral("X509 Certificate"));
#endif

    const QString quaServerRevision = QStringLiteral(OUASIM_QUASERVER_REVISION);
    ui->quaServerVersionValue->setText(quaServerRevision.isEmpty() ? tr("Not available") : quaServerRevision);
    ui->sdkValue->setText(QStringLiteral("open62541"));
    ui->sdkVersionValue->setText(QStringLiteral(UA_OPEN62541_VERSION));
    ui->qtVersionValue->setText(tr("%1 (built against %2)").arg(qVersion(), QStringLiteral(QT_VERSION_STR)));
    ui->securityPoliciesValue->setText(policies.join(QStringLiteral(", ")));
    ui->securityModesValue->setText(modes.join(QStringLiteral(", ")));
    ui->userTokenTypesValue->setText(tokens.join(QStringLiteral(", ")));
    ui->transportValue->setText(QStringLiteral("opc.tcp"));
    ui->roleValue->setText(tr("Server"));
}

///
/// \brief Emphasises the title and section headings.
///
void OpcUaInfoDialog::setupFonts()
{
    QFont titleFont = ui->titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    ui->titleLabel->setFont(titleFont);

    const QList<QLabel *> sectionLabels = {
        ui->stackTitleLabel,
        ui->securityTitleLabel,
        ui->protocolTitleLabel,
        ui->resourcesTitleLabel,
    };
    for (QLabel *label : sectionLabels) {
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
    }
}

///
/// \brief Aligns table value columns across independent sections.
///
void OpcUaInfoDialog::setupLayout()
{
    const QList<QGridLayout *> grids = {
        ui->stackGrid,
        ui->securityGrid,
        ui->protocolGrid,
        ui->resourcesGrid,
    };

    int captionColumnWidth = 0;
    for (QGridLayout *grid : grids) {
        for (int index = 0; index < grid->count(); ++index) {
            int row = 0;
            int column = 0;
            int rowSpan = 0;
            int columnSpan = 0;
            grid->getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
            if (column != 0)
                continue;
            if (const QWidget *caption = grid->itemAt(index)->widget())
                captionColumnWidth = qMax(captionColumnWidth, caption->sizeHint().width());
        }
    }

    for (QGridLayout *grid : grids) {
        grid->setColumnMinimumWidth(0, captionColumnWidth);
        grid->setColumnStretch(0, 0);
        grid->setColumnStretch(1, 1);
        for (int index = 0; index < grid->count(); ++index) {
            QLayoutItem *item = grid->itemAt(index);
            if (item)
                item->setAlignment(Qt::AlignTop);
        }
    }
}

///
/// \brief Loads the official OPC UA logo from application resources.
///
void OpcUaInfoDialog::setupLogo()
{
    const QString logoResource = QStringLiteral(":/branding/opcua-%1.png")
        .arg(AppPalette::isDark(palette()) ? QStringLiteral("dark") : QStringLiteral("light"));
    ui->logoLabel->setPixmap(QPixmap(logoResource));
}

///
/// \brief Configures external resource links.
///
void OpcUaInfoDialog::setupLinks()
{
    ui->foundationLinkLabel->setText(
        QStringLiteral("<a href=\"https://opcfoundation.org\">https://opcfoundation.org</a>"));
    ui->specificationLinkLabel->setText(
        QStringLiteral("<a href=\"https://reference.opcfoundation.org\">"
                       "https://reference.opcfoundation.org</a>"));

    const QList<QLabel *> linkLabels = { ui->foundationLinkLabel, ui->specificationLinkLabel };
    for (QLabel *label : linkLabels) {
        label->setTextFormat(Qt::RichText);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        label->setOpenExternalLinks(true);
    }
}

