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
#include <QShortcut>
#include <QTableWidget>
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
#include "valuedialog.h"
#include "valuepreviewdelegate.h"
#include "viewsetup.h"

namespace {

///
/// \brief Appends a field with a compact preview and preserves its complete value for selection.
/// \param view Certificate field list.
/// \param name Field label.
/// \param value Complete field value.
/// \param iconName Icon resource identifying the field.
///
void addDetail(QTableWidget *view, const QString &name, const QString &value, const QString &iconName)
{
    const int row = view->rowCount();
    view->insertRow(row);
    view->setItem(row, 0, new QTableWidgetItem(AppIcons::icon(iconName), name));
    auto *preview = new QTableWidgetItem(value.simplified());
    preview->setData(Qt::UserRole, value);
    view->setItem(row, 1, preview);
}

///
/// \brief Lists an extension only when it is present in the certificate.
/// \param view Certificate field list.
/// \param name Extension label.
/// \param value Decoded extension value.
///
void addExtension(QTableWidget *view, const QString &name, const QString &value)
{
    if (!value.isEmpty())
        addDetail(view, name, value, QStringLiteral("shield-check"));
}

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
    ViewSetup::configureTable(ui->detailsView);
    ui->detailsView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->detailsView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    auto *previewDelegate = new ValuePreviewDelegate(ui->detailsView);
    ui->detailsView->setItemDelegateForColumn(1, previewDelegate);
    fitDetailsWidth();

    ViewSetup::installDelegate(ui->categoriesList);
    ui->categoriesList->setModel(_categoriesModel);
    ViewSetup::configureTable(ui->certificatesView);
    ui->certificatesView->setModel(_certificatesModel);
    ViewSetup::fitColumnsWhenFilled(ui->certificatesView);

    connect(ui->categoriesList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &CertificatesPage::showCategory);
    connect(ui->certificatesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &CertificatesPage::showCertificate);
    connect(previewDelegate, &ValuePreviewDelegate::expandRequested, this, &CertificatesPage::showDetailValue);
    connect(ui->detailsView, &QTableWidget::activated, this, &CertificatesPage::showDetailValue);
    // Return does not activate table items on every platform (notably macOS).
    for (const auto key : { Qt::Key_Return, Qt::Key_Enter }) {
        auto *shortcut = new QShortcut(QKeySequence(key), ui->detailsView);
        shortcut->setContext(Qt::WidgetShortcut);
        connect(shortcut, &QShortcut::activated, this, [this] {
            showDetailValue(ui->detailsView->currentIndex());
        });
    }
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

    if (!ViewSetup::selectById(ui->categoriesList, previous.isEmpty() ? TreeBuilders::allCertificatesId() : previous))
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
/// \brief Retranslates the page when the language changes, keeping the selected folder and certificate.
/// \param event Change event.
///
void CertificatesPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        if (!CertificateGenerator::isAvailable())
            ui->generateButton->setToolTip(tr("This build has no encryption support"));
        const int certificateRow = ui->certificatesView->currentIndex().row();
        setCertificates(_certificates);
        if (certificateRow > 0)
            ViewSetup::selectRow(ui->certificatesView, certificateRow);
        _certificatesModel->retranslate();
        fitDetailsWidth();
    }
    QWidget::changeEvent(event);
}

///
/// \brief Keeps the details panel at least as wide as its contents in the current language.
///
void CertificatesPage::fitDetailsWidth()
{
    // The details never scroll sideways, so the splitter must not squeeze them below their width.
    ui->detailsPanelScroll->setMinimumWidth(ui->detailsPanelBody->minimumSizeHint().width()
                                            + ui->detailsPanelScroll->verticalScrollBar()->sizeHint().width());
}

///
/// \brief Lists the certificates of the selected store folder, or of every folder.
/// \param current Selected folder index.
///
void CertificatesPage::showCategory(const QModelIndex &current)
{
    const QString id = current.siblingAtColumn(0).data(ItemRoles::Id).toString();
    const bool all = id == TreeBuilders::allCertificatesId();
    bool ok = false;
    const int category = id.toInt(&ok);
    const bool ownFolder = ok && category == static_cast<int>(CertificateInfo::Category::Application);
    ui->generateButton->setVisible(ownFolder);
    ui->trustButton->setVisible(!ownFolder);
    ui->rejectButton->setVisible(!ownFolder);
    ui->importButton->setEnabled(ok);

    QList<CertificateInfo> shown;
    for (const CertificateInfo &certificate : std::as_const(_certificates)) {
        if (all || (ok && static_cast<int>(certificate.category) == category))
            shown.append(certificate);
    }
    _certificatesModel->setItems(shown);
    ui->certificatesTitle->setText(ok || all ? current.siblingAtColumn(0).data().toString()
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
    const int selectedField = qMax(0, ui->detailsView->currentRow());
    clearDetails();
    addDetail(ui->detailsView, tr("Version"), certificate.version, QStringLiteral("certificate"));
    addDetail(ui->detailsView, tr("Serial Number"), certificate.serialNumber, QStringLiteral("hash"));
    addDetail(ui->detailsView, tr("Signature Algorithm"), certificate.signatureAlgorithm, QStringLiteral("certificate"));
    addDetail(ui->detailsView, tr("Signature Hash Algorithm"), certificate.signatureHashAlgorithm, QStringLiteral("hash"));
    addDetail(ui->detailsView, tr("Issuer"), certificate.issuer, QStringLiteral("certificate"));
    addDetail(ui->detailsView, tr("Valid from"), Formatting::dateTime(certificate.validFrom), QStringLiteral("clock"));
    addDetail(ui->detailsView, tr("Valid to"), Formatting::dateTime(certificate.validTo), QStringLiteral("clock"));
    addDetail(ui->detailsView, tr("Subject"), certificate.subject, QStringLiteral("user"));
    addDetail(ui->detailsView, tr("Public Key"), certificate.publicKey, QStringLiteral("certificate"));
    if (!certificate.publicKeyParameters.isEmpty())
        addDetail(ui->detailsView, tr("Public Key Parameters"), certificate.publicKeyParameters, QStringLiteral("certificate"));
    addExtension(ui->detailsView, tr("Basic Constraints"), certificate.basicConstraints);
    addExtension(ui->detailsView, tr("Key Usage"), certificate.keyUsage);
    addExtension(ui->detailsView, tr("Extended Key Usage"), certificate.extendedKeyUsage);
    addExtension(ui->detailsView, tr("Subject Key Identifier"), certificate.subjectKeyIdentifier);
    addExtension(ui->detailsView, tr("Authority Key Identifier"), certificate.authorityKeyIdentifier);
    addExtension(ui->detailsView, tr("Subject Alternative Names"), certificate.subjectAlternativeNames);
    addDetail(ui->detailsView, tr("SHA-256 Fingerprint"), certificate.sha256Fingerprint, QStringLiteral("hash"));
    addDetail(ui->detailsView, tr("SHA-1 Thumbprint"), certificate.fingerprint, QStringLiteral("hash"));
    addDetail(ui->detailsView, tr("File Path"), certificate.filePath, QStringLiteral("folder"));
    addDetail(ui->detailsView, tr("X.509 Details"), certificate.details, QStringLiteral("certificate"));
    ui->detailsView->selectRow(selectedField);

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
    ui->detailsView->setRowCount(0);
}

///
/// \brief Opens the complete field value in a separate viewer.
/// \param index Activated field cell.
///
void CertificatesPage::showDetailValue(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    const int row = index.row();
    ui->detailsView->selectRow(row);
    auto *dialog = new ValueDialog(ui->detailsView->item(row, 0)->text(),
                                  ui->detailsView->item(row, 1)->data(Qt::UserRole).toString(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
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
