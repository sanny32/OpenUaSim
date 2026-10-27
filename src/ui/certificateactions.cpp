// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificateactions.cpp
/// \brief Implements the user actions of the Certificates page: import, export, trust, delete and generate.
///

#include <algorithm>

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>

#include "certificateactions.h"
#include "certificatespage.h"
#include "messagedialog.h"
#include "server/servercontroller.h"

namespace {

///
/// \brief Explains what deleting a certificate from a store folder changes.
/// \param category Folder of the certificate.
/// \return One or two sentences for the confirmation.
///
QString deletionConsequence(CertificateInfo::Category category)
{
    switch (category) {
    case CertificateInfo::Category::Application:
        return CertificateActions::tr("The server creates a new certificate on its next start. "
                                      "Clients that trust the current one will have to trust the new one.");
    case CertificateInfo::Category::Trusted:
        return CertificateActions::tr("The client will no longer be trusted.");
    case CertificateInfo::Category::Rejected:
        return CertificateActions::tr("If the client connects again, its certificate appears again.");
    case CertificateInfo::Category::Issuer:
        return CertificateActions::tr("Client certificates signed by this issuer will no longer "
                                      "be validated through it.");
    }
    return {};
}

}

///
/// \brief Wires the page to the controller and keeps the page showing the PKI store.
/// \param controller Controller owning the PKI store; must outlive the actions.
/// \param page Page whose requests are carried out.
/// \param window Parent of the dialogs and owner of the actions.
///
CertificateActions::CertificateActions(ServerController &controller, CertificatesPage *page, QWidget *window)
    : QObject(window)
    , _controller(controller)
    , _page(page)
    , _window(window)
{
    connect(page, &CertificatesPage::generateRequested, this, &CertificateActions::confirmGenerateServerCertificate);
    connect(page, &CertificatesPage::importRequested, this, &CertificateActions::importCertificate);
    connect(page, &CertificatesPage::filesDropped, this, &CertificateActions::importCertificateFiles);
    connect(page, &CertificatesPage::exportRequested, this, &CertificateActions::exportCertificate);
    connect(page, &CertificatesPage::moveRequested, this, &CertificateActions::moveCertificate);
    connect(page, &CertificatesPage::deleteRequested, this, &CertificateActions::confirmDeleteCertificate);
    connect(&controller, &ServerController::certificatesChanged, this, &CertificateActions::reloadCertificates);
}

///
/// \brief Adds a client or issuer certificate to the PKI store, reporting a refused file.
/// \param fileName PEM or DER certificate.
/// \param category Trusted, Rejected or Issuer folder.
/// \return True when the certificate was stored.
///
bool CertificateActions::importCertificateFrom(const QString &fileName, CertificateInfo::Category category)
{
    QString error;
    if (_controller.pki().importCertificate(fileName, category, &error))
        return true;
    MessageDialog::warning(tr("Certificate not imported"), error, _window);
    return false;
}

///
/// \brief Adds several client or issuer certificates to the PKI store, reporting every refused file at once.
/// \param fileNames PEM or DER certificates.
/// \param category Trusted, Rejected or Issuer folder.
/// \return Number of certificates stored.
///
int CertificateActions::importCertificateFiles(const QStringList &fileNames, CertificateInfo::Category category)
{
    int stored = 0;
    QStringList errors;
    for (const QString &fileName : fileNames) {
        QString error;
        if (_controller.pki().importCertificate(fileName, category, &error))
            ++stored;
        else
            errors << error;
    }
    if (!errors.isEmpty()) {
        MessageDialog::warning(errors.size() == 1 ? tr("Certificate not imported") : tr("Certificates not imported"),
                               errors.join(QLatin1Char('\n')), _window);
    }
    return stored;
}

///
/// \brief Installs the server certificate and key, reporting refused files.
/// \param certificateFile PEM or DER certificate.
/// \param keyFile PEM or DER private key.
/// \return True when both were stored.
///
bool CertificateActions::installServerCertificateFrom(const QString &certificateFile, const QString &keyFile)
{
    QString error;
    if (_controller.pki().installServerCertificate(certificateFile, keyFile, &error))
        return true;
    MessageDialog::warning(tr("Server certificate not installed"), error, _window);
    return false;
}

///
/// \brief Picks a certificate, and for the server folder also its private key, and stores it.
/// \param category Folder the certificate goes to.
///
void CertificateActions::importCertificate(CertificateInfo::Category category)
{
    const QString filter = tr("Certificates (*.der *.pem *.crt *.cer);;All files (*)");
    const QString certificate = QFileDialog::getOpenFileName(_window, tr("Import Certificate"), QString(), filter);
    if (certificate.isEmpty())
        return;
    if (category != CertificateInfo::Category::Application) {
        importCertificateFrom(certificate, category);
        return;
    }
    const QString key = QFileDialog::getOpenFileName(_window, tr("Private Key of the Server Certificate"), QString(),
                                                     tr("Private keys (*.pem *.key *.der);;All files (*)"));
    if (!key.isEmpty())
        installServerCertificateFrom(certificate, key);
}

///
/// \brief Saves a copy of a stored certificate.
/// \param filePath Stored certificate file.
///
void CertificateActions::exportCertificate(const QString &filePath)
{
    const QString target = QFileDialog::getSaveFileName(_window, tr("Export Certificate"),
                                                        QFileInfo(filePath).fileName(), tr("DER certificates (*.der)"));
    if (target.isEmpty())
        return;
    QFile::remove(target);
    if (!QFile::copy(filePath, target))
        MessageDialog::warning(tr("Certificate not exported"), tr("Cannot write '%1'").arg(target), _window);
}

///
/// \brief Trusts or rejects a stored certificate.
/// \param filePath Stored certificate file.
/// \param category Target folder.
///
void CertificateActions::moveCertificate(const QString &filePath, CertificateInfo::Category category)
{
    QString error;
    if (!_controller.pki().moveCertificate(filePath, category, &error))
        MessageDialog::warning(tr("Certificate not moved"), error, _window);
}

///
/// \brief Asks before deleting a stored certificate.
/// \param filePath Stored certificate file.
///
void CertificateActions::confirmDeleteCertificate(const QString &filePath)
{
    const QList<CertificateInfo> certificates = _controller.pki().certificates();
    const auto certificate =
        std::find_if(certificates.cbegin(), certificates.cend(),
                     [&filePath](const CertificateInfo &info) { return info.filePath == filePath; });
    if (certificate == certificates.cend())
        return;
    _pendingCertificate = filePath;
    auto *dialog = MessageDialog::confirmation(tr("Delete Certificate"),
                                               tr("Delete the certificate '%1'?").arg(certificate->commonName),
                                               deletionConsequence(certificate->category), tr("Delete"), _window);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &CertificateActions::deletePendingCertificate);
    dialog->open();
}

///
/// \brief Deletes the certificate the Delete Certificate question was confirmed for.
///
void CertificateActions::deletePendingCertificate()
{
    QString error;
    if (!_controller.pki().removeCertificate(_pendingCertificate, &error))
        MessageDialog::warning(tr("Certificate not deleted"), error, _window);
}

///
/// \brief Creates a server certificate, first asking before it replaces an installed one.
///
void CertificateActions::confirmGenerateServerCertificate()
{
    if (!_controller.pki().store().hasServerCertificate()) {
        generateServerCertificate();
        return;
    }
    auto *dialog = MessageDialog::confirmation(tr("Generate Server Certificate"),
                                               tr("Replace the installed server certificate?"),
                                               tr("Clients that trust it will have to trust the new one."),
                                               tr("Replace"), _window);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &CertificateActions::generateServerCertificate);
    dialog->open();
}

///
/// \brief Creates and installs a self-signed server certificate, reporting a failure.
///
void CertificateActions::generateServerCertificate()
{
    QString error;
    if (!_controller.generateServerCertificate(&error))
        MessageDialog::warning(tr("Server certificate not created"), error, _window);
}

///
/// \brief Shows the current contents of the PKI store.
///
void CertificateActions::reloadCertificates()
{
    _page->setCertificates(_controller.pki().certificates());
}
