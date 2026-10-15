// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatespage.cpp
/// \brief Implements the certificate store page.
///

#include <QHeaderView>

#include "appicons.h"
#include "certificatesmodel.h"
#include "certificatespage.h"
#include "formatting.h"
#include "itemroles.h"
#include "treebuilders.h"
#include "treemodel.h"
#include "ui_certificatespage.h"
#include "viewsetup.h"

///
/// \brief Builds the page.
/// \param parent Parent widget.
///
CertificatesPage::CertificatesPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CertificatesPage)
    , _categoriesModel(new TreeModel(this))
    , _certificatesModel(new CertificatesModel(this))
{
    ui->setupUi(this);
    ui->openFolderButton->setIcon(AppIcons::icon(QStringLiteral("import")));
    ui->importButton->setIcon(AppIcons::icon(QStringLiteral("download")));
    ui->exportButton->setIcon(AppIcons::icon(QStringLiteral("upload")));
    ui->trustButton->setIcon(AppIcons::icon(QStringLiteral("shield-check")));
    ui->rejectButton->setIcon(AppIcons::icon(QStringLiteral("shield-x")));

    ViewSetup::installDelegate(ui->categoriesList);
    ViewSetup::attachSearch(ui->categoriesList, _categoriesModel, ui->searchEdit);
    ViewSetup::configureTable(ui->certificatesView);
    ui->certificatesView->setModel(_certificatesModel);

    connect(ui->categoriesList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &CertificatesPage::showCategory);
    connect(ui->certificatesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &CertificatesPage::showCertificate);
}

///
/// \brief Destroys the page.
///
CertificatesPage::~CertificatesPage()
{
    delete ui;
}

///
/// \brief Shows the certificates of every store folder.
/// \param certificates Certificates to show.
///
void CertificatesPage::setCertificates(const QList<CertificateInfo> &certificates)
{
    _certificates = certificates;
    _categoriesModel->setNodes(TreeBuilders::certificateCategories(certificates));
    QHeaderView *header = ui->categoriesList->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->categoriesList->setCurrentIndex(QModelIndex());

    const QString trusted = QString::number(static_cast<int>(CertificateInfo::Category::Trusted));
    if (!ViewSetup::selectById(ui->categoriesList, trusted))
        ViewSetup::applyInitialState(ui->categoriesList);
}

///
/// \brief Returns the table model of the selected folder.
/// \return Model of the certificates in the selected folder.
///
const CertificatesModel *CertificatesPage::certificatesModel() const
{
    return _certificatesModel;
}

///
/// \brief Lists the certificates of the selected store folder.
/// \param current Selected folder index.
///
void CertificatesPage::showCategory(const QModelIndex &current)
{
    bool ok = false;
    const int category = current.siblingAtColumn(0).data(ItemRoles::Id).toInt(&ok);

    QList<CertificateInfo> shown;
    for (const CertificateInfo &certificate : std::as_const(_certificates)) {
        if (ok && static_cast<int>(certificate.category) == category)
            shown.append(certificate);
    }
    _certificatesModel->setItems(shown);
    ui->certificatesTitle->setText(ok ? current.siblingAtColumn(0).data().toString()
                                      : tr("Certificates"));
    ViewSetup::selectRow(ui->certificatesView, 0);
    if (shown.isEmpty())
        showCertificate(QModelIndex());
}

///
/// \brief Fills the details panel from the selected certificate.
/// \param current Selected table index.
///
void CertificatesPage::showCertificate(const QModelIndex &current)
{
    const bool valid = current.isValid();
    ui->detailsHost->setEnabled(valid);
    ui->exportButton->setEnabled(valid);
    if (!valid) {
        ui->trustButton->setEnabled(false);
        ui->rejectButton->setEnabled(false);
        return;
    }

    const CertificateInfo &certificate = _certificatesModel->item(current.row());
    ui->commonNameEdit->setText(certificate.commonName);
    ui->subjectEdit->setPlainText(certificate.subject);
    ui->issuerEdit->setPlainText(certificate.issuer);
    ui->serialNumberEdit->setText(certificate.serialNumber);
    ui->validFromEdit->setText(Formatting::dateTime(certificate.validFrom));
    ui->validToEdit->setText(Formatting::dateTime(certificate.validTo));
    ui->statusIndicator->setStatus(CertificatesModel::status(certificate.category),
                                   CertificatesModel::statusText(certificate.category));
    ui->keyUsageEdit->setText(certificate.keyUsage);
    ui->filePathEdit->setPlainText(certificate.filePath);

    const bool storeFolder = certificate.category == CertificateInfo::Category::Trusted
                             || certificate.category == CertificateInfo::Category::Rejected;
    ui->trustButton->setEnabled(storeFolder && certificate.category != CertificateInfo::Category::Trusted);
    ui->rejectButton->setEnabled(storeFolder && certificate.category != CertificateInfo::Category::Rejected);
}
