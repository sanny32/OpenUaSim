// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatespage.cpp
/// \brief Implements the certificate store page.
///

#include <QDesktopServices>
#include <QFileInfo>
#include <QHeaderView>
#include <QScrollBar>
#include <QUrl>

#include "appicons.h"
#include "certificatesmodel.h"
#include "certificatespage.h"
#include "formatting.h"
#include "itemroles.h"
#include "pki/certificategenerator.h"
#include "treebuilders.h"
#include "treemodel.h"
#include "ui_certificatespage.h"
#include "viewsetup.h"

namespace {

constexpr int kFilePathLines = 2;

}

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
    ViewSetup::splitByStretch(ui->splitter);
    ui->openFolderButton->setIcon(AppIcons::icon(QStringLiteral("import")));
    ui->generateButton->setIcon(AppIcons::icon(QStringLiteral("certificate")));
    ui->importButton->setIcon(AppIcons::icon(QStringLiteral("download")));
    ui->exportButton->setIcon(AppIcons::icon(QStringLiteral("upload")));
    ui->trustButton->setIcon(AppIcons::icon(QStringLiteral("shield-check")));
    ui->rejectButton->setIcon(AppIcons::icon(QStringLiteral("shield-x")));
    ui->deleteButton->setIcon(AppIcons::icon(QStringLiteral("trash")));
    ui->generateButton->setVisible(false);
    if (!CertificateGenerator::isAvailable()) {
        ui->generateButton->setEnabled(false);
        ui->generateButton->setToolTip(tr("This build has no encryption support"));
    }
    ViewSetup::fitHeightToLines(ui->subjectEdit, ui->subjectEdit->blockCount());
    ViewSetup::fitHeightToLines(ui->issuerEdit, ui->issuerEdit->blockCount());
    ViewSetup::fitHeightToLines(ui->filePathEdit, kFilePathLines);
    // The details never scroll sideways, so the splitter must not squeeze them below their width.
    ui->detailsPanelScroll->setMinimumWidth(ui->detailsPanelBody->minimumSizeHint().width()
                                            + ui->detailsPanelScroll->verticalScrollBar()->sizeHint().width());

    ViewSetup::installDelegate(ui->categoriesList);
    ViewSetup::attachSearch(ui->categoriesList, _categoriesModel, ui->searchEdit);
    ViewSetup::configureTable(ui->certificatesView);
    ui->certificatesView->setModel(_certificatesModel);
    ViewSetup::fitColumnsWhenFilled(ui->certificatesView);

    connect(ui->categoriesList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &CertificatesPage::showCategory);
    connect(ui->certificatesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &CertificatesPage::showCertificate);
    connect(ui->generateButton, &QPushButton::clicked, this, &CertificatesPage::generateRequested);
    connect(ui->importButton, &QPushButton::clicked, this, &CertificatesPage::requestImport);
    connect(ui->exportButton, &QPushButton::clicked, this, &CertificatesPage::requestExport);
    connect(ui->trustButton, &QPushButton::clicked, this, &CertificatesPage::requestTrust);
    connect(ui->rejectButton, &QPushButton::clicked, this, &CertificatesPage::requestReject);
    connect(ui->deleteButton, &QToolButton::clicked, this, &CertificatesPage::requestDelete);
    connect(ui->openFolderButton, &QToolButton::clicked, this, &CertificatesPage::openContainingFolder);
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
    const QString previous = ViewSetup::currentId(ui->categoriesList);
    _certificates = certificates;
    _categoriesModel->setNodes(TreeBuilders::certificateCategories(certificates));
    QHeaderView *header = ui->categoriesList->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->categoriesList->setCurrentIndex(QModelIndex());

    const QString trusted = QString::number(static_cast<int>(CertificateInfo::Category::Trusted));
    if (!ViewSetup::selectById(ui->categoriesList, previous.isEmpty() ? trusted : previous))
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
    const bool ownFolder = ok && category == static_cast<int>(CertificateInfo::Category::Application);
    ui->generateButton->setVisible(ownFolder);
    ui->trustButton->setVisible(!ownFolder);
    ui->rejectButton->setVisible(!ownFolder);

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
    ui->deleteButton->setEnabled(valid);
    ui->openFolderButton->setEnabled(valid);
    if (!valid) {
        ui->trustButton->setEnabled(false);
        ui->rejectButton->setEnabled(false);
        clearDetails();
        return;
    }

    const CertificateInfo &certificate = _certificatesModel->item(current.row());
    ui->subjectEdit->setPlainText(certificate.subject);
    ViewSetup::fitHeightToLines(ui->subjectEdit, ui->subjectEdit->blockCount());
    ui->issuerEdit->setPlainText(certificate.issuer);
    ViewSetup::fitHeightToLines(ui->issuerEdit, ui->issuerEdit->blockCount());
    ui->serialNumberEdit->setText(certificate.serialNumber);
    ui->sha1Edit->setText(certificate.fingerprint);
    ui->sha256Edit->setText(certificate.sha256Fingerprint);
    ui->certificateTextEdit->setPlainText(certificate.details);
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

///
/// \brief Empties the details panel so it does not keep showing a certificate of another folder.
///
void CertificatesPage::clearDetails()
{
    ui->subjectEdit->clear();
    ui->issuerEdit->clear();
    ui->serialNumberEdit->clear();
    ui->sha1Edit->clear();
    ui->sha256Edit->clear();
    ui->certificateTextEdit->clear();
    ui->validFromEdit->clear();
    ui->validToEdit->clear();
    ui->statusIndicator->setStatus(StatusKind::None, QString());
    ui->keyUsageEdit->clear();
    ui->filePathEdit->clear();
}

///
/// \brief Returns the selected store folder.
/// \return Category number, or -1 when no folder is selected.
///
int CertificatesPage::currentCategory() const
{
    bool ok = false;
    const int category = ViewSetup::currentId(ui->categoriesList).toInt(&ok);
    return ok ? category : -1;
}

///
/// \brief Returns the file of the selected certificate.
/// \return Path, or empty when no certificate is selected.
///
QString CertificatesPage::currentFilePath() const
{
    const QModelIndex current = ui->certificatesView->currentIndex();
    return current.isValid() ? _certificatesModel->item(current.row()).filePath : QString();
}

///
/// \brief Asks to import a certificate into the selected folder.
///
void CertificatesPage::requestImport()
{
    const int category = currentCategory();
    if (category >= 0)
        emit importRequested(CertificateInfo::Category(category));
}

///
/// \brief Asks to save a copy of the selected certificate.
///
void CertificatesPage::requestExport()
{
    const QString filePath = currentFilePath();
    if (!filePath.isEmpty())
        emit exportRequested(filePath);
}

///
/// \brief Asks to move the selected certificate to the trusted folder.
///
void CertificatesPage::requestTrust()
{
    const QString filePath = currentFilePath();
    if (!filePath.isEmpty())
        emit moveRequested(filePath, CertificateInfo::Category::Trusted);
}

///
/// \brief Asks to move the selected certificate to the rejected folder.
///
void CertificatesPage::requestReject()
{
    const QString filePath = currentFilePath();
    if (!filePath.isEmpty())
        emit moveRequested(filePath, CertificateInfo::Category::Rejected);
}

///
/// \brief Asks to delete the selected certificate.
///
void CertificatesPage::requestDelete()
{
    const QString filePath = currentFilePath();
    if (!filePath.isEmpty())
        emit deleteRequested(filePath);
}

///
/// \brief Shows the folder of the selected certificate in the file manager.
///
void CertificatesPage::openContainingFolder()
{
    const QString filePath = currentFilePath();
    if (!filePath.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
}
