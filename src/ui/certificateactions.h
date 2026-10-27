// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificateactions.h
/// \brief Declares the user actions of the Certificates page: import, export, trust, delete and generate.
///

#pragma once

#include <QObject>
#include <QStringList>

#include "data/certificateinfo.h"

class CertificatesPage;
class ServerController;
class QWidget;

///
/// \brief Carries out the requests of the Certificates page on the PKI store, asking before destructive changes
///        and reporting failures in message boxes.
///
class CertificateActions : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Wires the page to the controller and keeps the page showing the PKI store.
    /// \param controller Controller owning the PKI store; must outlive the actions.
    /// \param page Page whose requests are carried out.
    /// \param window Parent of the dialogs and owner of the actions.
    ///
    CertificateActions(ServerController &controller, CertificatesPage *page, QWidget *window);

    ///
    /// \brief Adds a client or issuer certificate to the PKI store, reporting a refused file.
    /// \param fileName PEM or DER certificate.
    /// \param category Trusted, Rejected or Issuer folder.
    /// \return True when the certificate was stored.
    ///
    bool importCertificateFrom(const QString &fileName, CertificateInfo::Category category);

    ///
    /// \brief Adds several client or issuer certificates to the PKI store, reporting every refused file at once.
    /// \param fileNames PEM or DER certificates.
    /// \param category Trusted, Rejected or Issuer folder.
    /// \return Number of certificates stored.
    ///
    int importCertificateFiles(const QStringList &fileNames, CertificateInfo::Category category);

    ///
    /// \brief Installs the server certificate and key, reporting refused files.
    /// \param certificateFile PEM or DER certificate.
    /// \param keyFile PEM or DER private key.
    /// \return True when both were stored.
    ///
    bool installServerCertificateFrom(const QString &certificateFile, const QString &keyFile);

private:
    void importCertificate(CertificateInfo::Category category);
    void exportCertificate(const QString &filePath);
    void moveCertificate(const QString &filePath, CertificateInfo::Category category);
    void confirmDeleteCertificate(const QString &filePath);
    void deletePendingCertificate();
    void confirmGenerateServerCertificate();
    void generateServerCertificate();
    void reloadCertificates();

    ServerController &_controller;
    CertificatesPage *_page;
    QWidget *_window;
    QString _pendingCertificate;
};
