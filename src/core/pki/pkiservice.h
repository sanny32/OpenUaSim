// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file pkiservice.h
/// \brief Declares the PKI operations of the served project: certificate store changes and client reviews.
///

#pragma once

#include <QObject>
#include <QStringList>

#include "data/logentry.h"
#include "pki/certificatestore.h"

class SimServer;

///
/// \brief Changes the PKI store, gives the changes to the server and keeps rejected client certificates for review.
///
class PkiService : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Creates the service on the default PKI store.
    /// \param server Server receiving the certificates; must outlive the service.
    /// \param parent Owning QObject.
    ///
    explicit PkiService(SimServer &server, QObject *parent = nullptr);

    ///
    /// \brief Returns the PKI store, e.g. to point tests at a temporary directory.
    /// \return Store owned by the service.
    ///
    CertificateStore &store();

    ///
    /// \brief Returns the PKI store.
    /// \return Store owned by the service.
    ///
    const CertificateStore &store() const;

    ///
    /// \brief Lists the certificates of the PKI store.
    /// \return Certificates of every folder.
    ///
    QList<CertificateInfo> certificates() const;

    ///
    /// \brief Adds a client or issuer certificate to the PKI store.
    /// \param fileName PEM or DER certificate.
    /// \param category Trusted, Rejected or Issuer folder.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the certificate was stored.
    ///
    bool importCertificate(const QString &fileName, CertificateInfo::Category category, QString *error = nullptr);

    ///
    /// \brief Installs the server certificate and private key used by secure endpoints.
    /// \param certificateFile PEM or DER certificate.
    /// \param keyFile PEM or DER private key.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when both were stored.
    ///
    bool installServerCertificate(const QString &certificateFile, const QString &keyFile, QString *error = nullptr);

    ///
    /// \brief Creates a self-signed server certificate and installs it, replacing an earlier one.
    /// \param applicationUri Application URI the certificate names.
    /// \param hostnames Host names the certificate names.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the certificate was created and stored.
    ///
    bool generateServerCertificate(const QString &applicationUri, const QStringList &hostnames,
                                   QString *error = nullptr);

    ///
    /// \brief Creates a missing server certificate when generation is available and logs the result.
    /// \param applicationUri Application URI the certificate names.
    /// \param hostnames Host names the certificate names.
    ///
    void ensureServerCertificate(const QString &applicationUri, const QStringList &hostnames);

    ///
    /// \brief Moves a stored certificate, e.g. to trust a rejected client.
    /// \param filePath Stored certificate file.
    /// \param category Target folder.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the certificate was moved.
    ///
    bool moveCertificate(const QString &filePath, CertificateInfo::Category category, QString *error = nullptr);

    ///
    /// \brief Deletes a stored certificate.
    /// \param filePath Stored certificate file.
    /// \param error Receives the reason on failure; may be null.
    /// \return True when the certificate was deleted.
    ///
    bool removeCertificate(const QString &filePath, QString *error = nullptr);

signals:
    ///
    /// \brief Announces a change of the PKI store.
    ///
    void certificatesChanged();

    ///
    /// \brief Announces a change the running server applies only after a restart, i.e. another server certificate.
    ///
    void restartRequired();

    ///
    /// \brief Announces a PKI event for the server log.
    /// \param entry Log entry.
    ///
    void logAppended(const LogEntry &entry);

private:
    bool finishChange(bool changed);
    void log(LogEntry::Level level, const QString &message);
    void handleCertificateRejected(const QByteArray &certificate, const QString &reason);
    void handleUnverifiedClient(const QByteArray &certificate);

    SimServer &_server;
    CertificateStore _store;
};
