// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file pkiservice.cpp
/// \brief Implements the PKI operations of the served project: certificate store changes and client reviews.
///

#include <QFileInfo>

#include "pki/certificategenerator.h"
#include "pkiservice.h"
#include "server/logmapper.h"
#include "server/simserver.h"

///
/// \brief Creates the service on the default PKI store.
/// \param server Server receiving the certificates; must outlive the service.
/// \param parent Owning QObject.
///
PkiService::PkiService(SimServer &server, QObject *parent)
    : QObject(parent)
    , _server(server)
{
    connect(&_server, &SimServer::clientCertificateRejected, this, &PkiService::handleCertificateRejected);
    connect(&_server, &SimServer::unverifiedClientConnected, this, &PkiService::handleUnverifiedClient);
}

///
/// \brief Returns the PKI store, e.g. to point tests at a temporary directory.
/// \return Store owned by the service.
///
CertificateStore &PkiService::store()
{
    return _store;
}

///
/// \brief Returns the PKI store.
/// \return Store owned by the service.
///
const CertificateStore &PkiService::store() const
{
    return _store;
}

///
/// \brief Lists the certificates of the PKI store.
/// \return Certificates of every folder.
///
QList<CertificateInfo> PkiService::certificates() const
{
    return _store.certificates();
}

///
/// \brief Adds a client or issuer certificate to the PKI store.
/// \param fileName PEM or DER certificate.
/// \param category Trusted, Rejected or Issuer folder.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was stored.
///
bool PkiService::importCertificate(const QString &fileName, CertificateInfo::Category category, QString *error)
{
    return finishChange(!_store.importCertificate(fileName, category, error).isEmpty());
}

///
/// \brief Installs the server certificate and private key used by secure endpoints.
/// \param certificateFile PEM or DER certificate.
/// \param keyFile PEM or DER private key.
/// \param error Receives the reason on failure; may be null.
/// \return True when both were stored.
///
bool PkiService::installServerCertificate(const QString &certificateFile, const QString &keyFile, QString *error)
{
    return finishChange(_store.installServerCertificate(certificateFile, keyFile, error));
}

///
/// \brief Creates a self-signed server certificate and installs it, replacing an earlier one.
/// \param applicationUri Application URI the certificate names.
/// \param hostnames Host names the certificate names.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was created and stored.
///
bool PkiService::generateServerCertificate(const QString &applicationUri, const QStringList &hostnames,
                                           QString *error)
{
    const GeneratedCertificate generated = CertificateGenerator::generate(applicationUri, hostnames, error);
    return finishChange(!generated.certificate.isEmpty()
                        && _store.storeServerCertificate(generated.certificate, generated.privateKey, error));
}

///
/// \brief Creates a missing server certificate when generation is available and logs the result.
/// \param applicationUri Application URI the certificate names.
/// \param hostnames Host names the certificate names.
///
void PkiService::ensureServerCertificate(const QString &applicationUri, const QStringList &hostnames)
{
    if (!CertificateGenerator::isAvailable() || _store.hasServerCertificate())
        return;
    QString error;
    if (generateServerCertificate(applicationUri, hostnames, &error))
        log(LogEntry::Level::Info, tr("Created a self-signed server certificate"));
    else
        log(LogEntry::Level::Error, error);
}

///
/// \brief Moves a stored certificate, e.g. to trust a rejected client.
/// \param filePath Stored certificate file.
/// \param category Target folder.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was moved.
///
bool PkiService::moveCertificate(const QString &filePath, CertificateInfo::Category category, QString *error)
{
    return finishChange(!_store.move(filePath, category, error).isEmpty());
}

///
/// \brief Deletes a stored certificate.
/// \param filePath Stored certificate file.
/// \param error Receives the reason on failure; may be null.
/// \return True when the certificate was deleted.
///
bool PkiService::removeCertificate(const QString &filePath, QString *error)
{
    return finishChange(_store.remove(filePath, error));
}

///
/// \brief Gives a successful PKI change to the server and announces it; a running server applies the trust
///        lists at once and needs a restart only for another server certificate.
/// \param changed Whether the store changed.
/// \return The value of changed, so callers can return it directly.
///
bool PkiService::finishChange(bool changed)
{
    if (!changed)
        return false;
    if (!_server.setPkiMaterial(_store.material()))
        emit restartRequired();
    emit certificatesChanged();
    return true;
}

///
/// \brief Announces a log entry of the security category.
/// \param level Severity.
/// \param message Text of the entry.
///
void PkiService::log(LogEntry::Level level, const QString &message)
{
    emit logAppended(LogMapper::entry(level, LogEntry::Category::Security, message));
}

///
/// \brief Keeps a rejected client certificate in the rejected folder, where it can be reviewed and trusted;
///        a certificate the store already holds is not copied, and the log says why a trusted one failed.
/// \param certificate Rejected DER certificate.
/// \param reason Status code name of the rejection.
///
void PkiService::handleCertificateRejected(const QByteArray &certificate, const QString &reason)
{
    if (const std::optional<CertificateInfo> known = _store.find(certificate)) {
        const QString message = known->category == CertificateInfo::Category::Trusted
            ? tr("Client certificate '%1' rejected (%2) although it is trusted")
            : tr("Client certificate '%1' rejected again (%2)");
        log(LogEntry::Level::Warning, message.arg(known->commonName, reason));
        return;
    }
    QString error;
    const QString stored = _store.addCertificate(certificate, CertificateInfo::Category::Rejected, &error);
    if (stored.isEmpty()) {
        log(LogEntry::Level::Error, tr("A rejected client certificate could not be kept: %1").arg(error));
        return;
    }
    log(LogEntry::Level::Warning, tr("Client certificate '%1' rejected (%2); it was moved to the rejected folder")
                                      .arg(QFileInfo(stored).completeBaseName(), reason));
    emit certificatesChanged();
}

///
/// \brief Keeps the unknown certificate of a client accepted without a trust check in the rejected folder,
///        so it shows up for review and is not trusted until the user says so.
/// \param certificate DER application certificate of the client.
///
void PkiService::handleUnverifiedClient(const QByteArray &certificate)
{
    if (_store.find(certificate))
        return;
    QString error;
    const QString stored = _store.addCertificate(certificate, CertificateInfo::Category::Rejected, &error);
    if (stored.isEmpty()) {
        log(LogEntry::Level::Error, tr("The certificate of a connected client could not be kept: %1").arg(error));
        return;
    }
    log(LogEntry::Level::Info, tr("Client certificate '%1' accepted without a trust check; "
                                  "it was added to the rejected folder")
                                   .arg(QFileInfo(stored).completeBaseName()));
    emit certificatesChanged();
}
