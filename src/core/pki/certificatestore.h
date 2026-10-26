// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatestore.h
/// \brief Declares the folder-based PKI store of the server certificate and the client trust lists.
///

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include "data/certificateinfo.h"

///
/// \brief Certificate material QUaServer needs to offer secure endpoints.
///
struct PkiMaterial
{
    /// \brief Server certificate, DER or PEM; empty when none is installed.
    QByteArray certificate;
    /// \brief Private key of the server certificate, DER or PEM.
    QByteArray privateKey;
    /// \brief Trusted client certificates.
    QList<QByteArray> trusted;
    /// \brief Issuer certificates that complete client certificate chains.
    QList<QByteArray> issuers;
};

///
/// \brief PKI store kept as folders: own (server certificate and key), trusted, rejected and issuers.
///
class CertificateStore
{
public:
    ///
    /// \brief Opens a store.
    /// \param root Store directory; the folders are created on first write.
    ///
    explicit CertificateStore(const QString &root = defaultRoot());

    ///
    /// \brief Returns the store directory used by the application.
    /// \return "pki" below the application data directory.
    ///
    static QString defaultRoot();

    ///
    /// \brief Returns the store directory.
    /// \return Root path.
    ///
    QString root() const;

    ///
    /// \brief Points the store at another directory.
    /// \param root Store directory.
    ///
    void setRoot(const QString &root);

    ///
    /// \brief Returns the directory of a store folder.
    /// \param category Store folder.
    /// \return Path such as "<root>/trusted/certs".
    ///
    QString folder(CertificateInfo::Category category) const;

    ///
    /// \brief Lists the certificates of every folder.
    /// \return Parsed certificates; files that are not certificates are skipped.
    ///
    QList<CertificateInfo> certificates() const;

    ///
    /// \brief Tells whether any folder already holds a certificate, whatever its file name.
    /// \param certificate PEM or DER certificate.
    /// \return False when no folder holds it or the data is not a certificate.
    ///
    bool isStored(const QByteArray &certificate) const;

    ///
    /// \brief Copies a certificate into a folder as DER.
    /// \param fileName PEM or DER certificate file.
    /// \param category Trusted, Rejected or Issuer folder.
    /// \param error Receives the reason on failure; may be null.
    /// \return Path of the stored file, or empty when the file is not a certificate or cannot be stored.
    ///
    QString importCertificate(const QString &fileName, CertificateInfo::Category category,
                              QString *error = nullptr);

    ///
    /// \brief Stores a certificate held in memory into a folder as DER.
    /// \param certificate PEM or DER certificate.
    /// \param category Trusted, Rejected or Issuer folder.
    /// \param error Receives the reason on failure; may be null.
    /// \return Path of the stored file, or empty when the data is not a certificate or cannot be stored.
    ///
    QString addCertificate(const QByteArray &certificate, CertificateInfo::Category category,
                           QString *error = nullptr);

    ///
    /// \brief Installs the server certificate and its private key, replacing earlier ones.
    /// \param certificateFile PEM or DER certificate.
    /// \param keyFile PEM or DER private key.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the certificate cannot be parsed, the key file is empty or the files cannot be stored.
    ///
    bool installServerCertificate(const QString &certificateFile, const QString &keyFile, QString *error = nullptr);

    ///
    /// \brief Installs a server certificate and private key held in memory, replacing earlier ones.
    /// \param certificate PEM or DER certificate.
    /// \param privateKey PEM or DER private key.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the certificate cannot be parsed, the key is empty or the files cannot be stored or renamed.
    ///
    bool storeServerCertificate(const QByteArray &certificate, const QByteArray &privateKey, QString *error = nullptr);

    ///
    /// \brief Tells whether a server certificate and its private key are installed.
    /// \return True when both files exist.
    ///
    bool hasServerCertificate() const;

    ///
    /// \brief Moves a stored certificate to another folder, e.g. to trust a rejected client.
    /// \param filePath Stored certificate file.
    /// \param category Target folder.
    /// \param error Receives the reason on failure; may be null.
    /// \return Path of the moved file, or empty when it is not in the store or cannot be moved.
    ///
    QString move(const QString &filePath, CertificateInfo::Category category, QString *error = nullptr);

    ///
    /// \brief Deletes a stored certificate; deleting the server certificate also deletes its key.
    /// \param filePath Stored certificate file.
    /// \param error Receives the reason on failure; may be null.
    /// \return False when the file is not in the store or cannot be deleted.
    ///
    bool remove(const QString &filePath, QString *error = nullptr);

    ///
    /// \brief Reads what QUaServer needs for secure endpoints.
    /// \return Server certificate and key with the trusted and issuer certificates.
    ///
    PkiMaterial material() const;

private:
    QString generatedCertificatePath() const;
    QString serverCertificatePath() const;
    QString keyPath() const;
    bool contains(const QString &filePath) const;

    QString _root;
};
