// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatestore.cpp
/// \brief Implements the folder-based PKI store of the server certificate and the client trust lists.
///

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QRegularExpression>
#include <QSslCertificate>
#include <QSslCertificateExtension>
#include <QSslKey>
#include <QStandardPaths>

#ifdef OUASIM_HAVE_ENCRYPTION
#include <memory>
#include <openssl/x509v3.h>
#endif

#include "certificatestore.h"

namespace {

///
/// \brief Translates a message of the certificate store.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("CertificateStore", text);
}

///
/// \brief Stores a failure reason when the caller asked for one.
/// \param error Destination; may be null.
/// \param message Reason.
///
void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

///
/// \brief Returns every store folder in display order.
/// \return Application, Trusted, Rejected and Issuer.
///
QList<CertificateInfo::Category> categories()
{
    return { CertificateInfo::Category::Application, CertificateInfo::Category::Trusted,
             CertificateInfo::Category::Rejected, CertificateInfo::Category::Issuer };
}

///
/// \brief Reads a file.
/// \param fileName File to read.
/// \return Contents, or empty when the file cannot be read.
///
QByteArray readFile(const QString &fileName)
{
    QFile file(fileName);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

///
/// \brief Parses a PEM or DER certificate.
/// \param data File contents.
/// \return Certificate; null when the data is neither.
///
QSslCertificate parseCertificate(const QByteArray &data)
{
    const QSslCertificate pem(data, QSsl::Pem);
    return pem.isNull() ? QSslCertificate(data, QSsl::Der) : pem;
}

///
/// \brief Formats a distinguished name one attribute per line.
/// \param certificate Certificate to describe.
/// \param subject True for the subject, false for the issuer.
/// \return Text such as "CN=Server,\nO=Company,\nC=US".
///
QString distinguishedName(const QSslCertificate &certificate, bool subject)
{
    const QList<QPair<QSslCertificate::SubjectInfo, QString>> attributes = {
        { QSslCertificate::CommonName, QStringLiteral("CN") },
        { QSslCertificate::OrganizationalUnitName, QStringLiteral("OU") },
        { QSslCertificate::Organization, QStringLiteral("O") },
        { QSslCertificate::LocalityName, QStringLiteral("L") },
        { QSslCertificate::StateOrProvinceName, QStringLiteral("ST") },
        { QSslCertificate::CountryName, QStringLiteral("C") },
    };
    QStringList parts;
    for (const auto &[attribute, key] : attributes) {
        const QStringList values = subject ? certificate.subjectInfo(attribute) : certificate.issuerInfo(attribute);
        for (const QString &value : values)
            parts.append(QStringLiteral("%1=%2").arg(key, value));
    }
    return parts.join(QStringLiteral(",\n"));
}

///
/// \brief Formats the public key algorithm and size.
/// \param key Certificate public key.
/// \return Algorithm and size in bits, or empty for an unavailable key.
///
QString publicKeyDescription(const QSslKey &key)
{
    if (key.isNull())
        return {};
    QString algorithm;
    switch (key.algorithm()) {
    case QSsl::Rsa: algorithm = QStringLiteral("RSA"); break;
    case QSsl::Dsa: algorithm = QStringLiteral("DSA"); break;
    case QSsl::Ec: algorithm = QStringLiteral("EC"); break;
    case QSsl::Dh: algorithm = QStringLiteral("DH"); break;
    case QSsl::Opaque: return tr("Opaque public key");
    }
    return tr("%1 (%2 bits)").arg(algorithm).arg(key.length());
}

#ifdef OUASIM_HAVE_ENCRYPTION
///
/// \brief Decodes a certificate extension into readable text.
/// \param certificate Parsed certificate.
/// \param nid OpenSSL extension identifier.
/// \return Extension text, or empty when absent or undecodable.
///
QString extensionText(X509 *certificate, int nid)
{
    const int index = X509_get_ext_by_NID(certificate, nid, -1);
    if (index < 0)
        return {};
    const std::unique_ptr<BIO, decltype(&BIO_free)> output(BIO_new(BIO_s_mem()), BIO_free);
    if (!output || X509V3_EXT_print(output.get(), X509_get_ext(certificate, index), 0, 0) != 1)
        return {};
    char *text = nullptr;
    const long size = BIO_get_mem_data(output.get(), &text);
    return QString::fromUtf8(text, size);
}

///
/// \brief Extracts the signature, key parameters and X.509 extensions.
/// \param certificate Certificate to describe.
/// \param info Destination for the decoded fields.
///
void describeCryptography(const QSslCertificate &certificate, CertificateInfo &info)
{
    const QByteArray der = certificate.toDer();
    const unsigned char *data = reinterpret_cast<const unsigned char *>(der.constData());
    const std::unique_ptr<X509, decltype(&X509_free)> parsed(d2i_X509(nullptr, &data, long(der.size())), X509_free);
    if (!parsed)
        return;
    // The native macOS TLS backend does not implement QSslCertificate::toText().
    const std::unique_ptr<BIO, decltype(&BIO_free)> output(BIO_new(BIO_s_mem()), BIO_free);
    if (output && X509_print(output.get(), parsed.get()) == 1) {
        char *text = nullptr;
        const long size = BIO_get_mem_data(output.get(), &text);
        info.details = QString::fromUtf8(text, size);
    }
    const int signature = X509_get_signature_nid(parsed.get());
    if (signature != NID_undef)
        info.signatureAlgorithm = QString::fromLatin1(OBJ_nid2ln(signature));
    int digest = NID_undef;
    if (X509_get_signature_info(parsed.get(), &digest, nullptr, nullptr, nullptr) == 1 && digest != NID_undef)
        info.signatureHashAlgorithm = QString::fromLatin1(OBJ_nid2sn(digest));

    X509_ALGOR *algorithm = nullptr;
    if (X509_PUBKEY_get0_param(nullptr, nullptr, nullptr, &algorithm, X509_get_X509_PUBKEY(parsed.get())) == 1
        && algorithm && algorithm->parameter) {
        const int size = i2d_ASN1_TYPE(algorithm->parameter, nullptr);
        if (size > 0) {
            QByteArray parameters(size, Qt::Uninitialized);
            unsigned char *output = reinterpret_cast<unsigned char *>(parameters.data());
            if (i2d_ASN1_TYPE(algorithm->parameter, &output) == size)
                info.publicKeyParameters = QString::fromLatin1(parameters.toHex(' ').toUpper());
        }
    }
    info.basicConstraints = extensionText(parsed.get(), NID_basic_constraints);
    info.keyUsage = extensionText(parsed.get(), NID_key_usage);
    info.extendedKeyUsage = extensionText(parsed.get(), NID_ext_key_usage);
    info.subjectKeyIdentifier = extensionText(parsed.get(), NID_subject_key_identifier);
    info.authorityKeyIdentifier = extensionText(parsed.get(), NID_authority_key_identifier);
    info.subjectAlternativeNames = extensionText(parsed.get(), NID_subject_alt_name);
}
#else
///
/// \brief Returns key usage exposed by the Qt TLS backend.
/// \param certificate Certificate to describe.
/// \return Usage names, or empty when unavailable.
///
QString keyUsage(const QSslCertificate &certificate)
{
    const QList<QSslCertificateExtension> extensions = certificate.extensions();
    for (const QSslCertificateExtension &extension : extensions) {
        if (extension.name() != QLatin1String("keyUsage"))
            continue;
        const QVariant value = extension.value();
        return value.typeId() == QMetaType::QStringList ? value.toStringList().join(QStringLiteral(", "))
                                                        : value.toString();
    }
    return {};
}
#endif

///
/// \brief Formats bytes as upper-case hex with a separator.
/// \param bytes Bytes to format.
/// \param separator Character between bytes.
/// \return Text such as "0A:1B:2C".
///
QString hex(const QByteArray &bytes, char separator)
{
    return QString::fromLatin1(bytes.toHex(separator).toUpper());
}

///
/// \brief Describes a certificate for the certificates page.
/// \param certificate Parsed certificate.
/// \param category Folder holding it.
/// \param filePath File holding it.
/// \return Description.
///
CertificateInfo describe(const QSslCertificate &certificate, CertificateInfo::Category category,
                         const QString &filePath)
{
    CertificateInfo info;
    info.category = category;
    info.commonName = certificate.subjectInfo(QSslCertificate::CommonName).value(0);
    info.version = QStringLiteral("V%1").arg(QString::fromLatin1(certificate.version()));
    info.publicKey = publicKeyDescription(certificate.publicKey());
    info.subject = distinguishedName(certificate, true);
    info.issuer = distinguishedName(certificate, false);
    info.serialNumber = QString::fromLatin1(certificate.serialNumber()).replace(QLatin1Char(':'), QLatin1Char(' ')).toUpper();
    info.validFrom = certificate.effectiveDate().toLocalTime();
    info.validTo = certificate.expiryDate().toLocalTime();
    info.fingerprint = hex(certificate.digest(QCryptographicHash::Sha1), ':');
    info.sha256Fingerprint = hex(certificate.digest(QCryptographicHash::Sha256), ':');
#ifdef OUASIM_HAVE_ENCRYPTION
    describeCryptography(certificate, info);
#else
    info.details = certificate.toText();
    info.keyUsage = keyUsage(certificate);
#endif
    info.filePath = QDir::toNativeSeparators(filePath);
    return info;
}

///
/// \brief Builds the store file name of a certificate.
/// \param certificate Parsed certificate.
/// \return Name such as "Test Client [3546E202...].der" with characters unsafe in file names replaced.
///
QString storedFileName(const QSslCertificate &certificate)
{
    static const QRegularExpression unsafe(QStringLiteral("[\\\\/:*?\"<>|]"));
    QString name = certificate.subjectInfo(QSslCertificate::CommonName).value(0);
    name.replace(unsafe, QStringLiteral("_"));
    const QString thumbprint = QString::fromLatin1(certificate.digest(QCryptographicHash::Sha1).toHex().toUpper());
    return QStringLiteral("%1 [%2].der").arg(name.isEmpty() ? QStringLiteral("certificate") : name, thumbprint);
}

///
/// \brief Writes bytes to a file, creating its directory.
/// \param fileName Target path.
/// \param data Contents.
/// \param error Receives the reason on failure; may be null.
/// \return True when the file was written.
///
bool writeFile(const QString &fileName, const QByteArray &data, QString *error)
{
    QDir().mkpath(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size())
        return true;
    setError(error, tr("Cannot write '%1': %2").arg(fileName, file.errorString()));
    return false;
}

}

///
/// \brief Opens a store.
/// \param root Store directory; the folders are created on first write.
///
CertificateStore::CertificateStore(const QString &root)
    : _root(root)
{
}

///
/// \brief Returns the store directory used by the application.
/// \return "pki" below the application data directory.
///
QString CertificateStore::defaultRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/pki");
}

///
/// \brief Returns the store directory.
/// \return Root path.
///
QString CertificateStore::root() const
{
    return _root;
}

///
/// \brief Points the store at another directory.
/// \param root Store directory.
///
void CertificateStore::setRoot(const QString &root)
{
    _root = root;
}

///
/// \brief Returns the directory of a store folder.
/// \param category Store folder.
/// \return Path such as "<root>/trusted/certs".
///
QString CertificateStore::folder(CertificateInfo::Category category) const
{
    switch (category) {
    case CertificateInfo::Category::Application: return _root + QStringLiteral("/own/certs");
    case CertificateInfo::Category::Trusted:     return _root + QStringLiteral("/trusted/certs");
    case CertificateInfo::Category::Rejected:    return _root + QStringLiteral("/rejected/certs");
    case CertificateInfo::Category::Issuer:      return _root + QStringLiteral("/issuers/certs");
    }
    return {};
}

///
/// \brief Lists the certificates of every folder.
/// \return Parsed certificates; files that are not certificates are skipped.
///
QList<CertificateInfo> CertificateStore::certificates() const
{
    QList<CertificateInfo> result;
    for (CertificateInfo::Category category : categories()) {
        const QFileInfoList files = QDir(folder(category)).entryInfoList(QDir::Files, QDir::Name);
        for (const QFileInfo &file : files) {
            const QSslCertificate certificate = parseCertificate(readFile(file.absoluteFilePath()));
            if (!certificate.isNull())
                result.append(describe(certificate, category, file.absoluteFilePath()));
        }
    }
    return result;
}

///
/// \brief Tells whether any folder already holds a certificate, whatever its file name.
/// \param certificate PEM or DER certificate.
/// \return False when no folder holds it or the data is not a certificate.
///
bool CertificateStore::isStored(const QByteArray &certificate) const
{
    const QSslCertificate wanted = parseCertificate(certificate);
    if (wanted.isNull())
        return false;
    for (CertificateInfo::Category category : categories()) {
        const QFileInfoList files = QDir(folder(category)).entryInfoList(QDir::Files);
        for (const QFileInfo &file : files) {
            if (parseCertificate(readFile(file.absoluteFilePath())) == wanted)
                return true;
        }
    }
    return false;
}

///
/// \brief Copies a certificate into a folder as DER.
/// \param fileName PEM or DER certificate file.
/// \param category Trusted, Rejected or Issuer folder.
/// \param error Receives the reason on failure; may be null.
/// \return Path of the stored file, or empty when the file is not a certificate or cannot be stored.
///
QString CertificateStore::importCertificate(const QString &fileName, CertificateInfo::Category category,
                                            QString *error)
{
    const QByteArray data = readFile(fileName);
    if (category != CertificateInfo::Category::Application && parseCertificate(data).isNull()) {
        setError(error, tr("'%1' is not a PEM or DER certificate").arg(fileName));
        return {};
    }
    return addCertificate(data, category, error);
}

///
/// \brief Stores a certificate held in memory into a folder as DER.
/// \param certificate PEM or DER certificate.
/// \param category Trusted, Rejected or Issuer folder.
/// \param error Receives the reason on failure; may be null.
/// \return Path of the stored file, or empty when the data is not a certificate or cannot be stored.
///
QString CertificateStore::addCertificate(const QByteArray &certificate, CertificateInfo::Category category,
                                         QString *error)
{
    if (category == CertificateInfo::Category::Application) {
        setError(error, tr("The server certificate is installed together with its private key"));
        return {};
    }
    const QSslCertificate parsed = parseCertificate(certificate);
    if (parsed.isNull()) {
        setError(error, tr("The data is not a PEM or DER certificate"));
        return {};
    }
    const QString target = folder(category) + QLatin1Char('/') + storedFileName(parsed);
    return writeFile(target, parsed.toDer(), error) ? target : QString();
}

///
/// \brief Installs the server certificate and its private key, replacing earlier ones.
/// \param certificateFile PEM or DER certificate.
/// \param keyFile PEM or DER private key.
/// \param error Receives the reason on failure; may be null.
/// \return False when the certificate cannot be parsed, the key file is empty or the files cannot be stored.
///
bool CertificateStore::installServerCertificate(const QString &certificateFile, const QString &keyFile,
                                                QString *error)
{
    const QByteArray certificate = readFile(certificateFile);
    if (parseCertificate(certificate).isNull()) {
        setError(error, tr("'%1' is not a PEM or DER certificate").arg(certificateFile));
        return false;
    }
    const QByteArray key = readFile(keyFile);
    if (key.isEmpty()) {
        setError(error, tr("'%1' holds no private key").arg(keyFile));
        return false;
    }
    return storeServerCertificate(certificate, key, error);
}

///
/// \brief Installs a server certificate and private key held in memory, replacing earlier ones.
/// \param certificate PEM or DER certificate.
/// \param privateKey PEM or DER private key.
/// \param error Receives the reason on failure; may be null.
/// \return False when the certificate cannot be parsed, the key is empty or the files cannot be stored or renamed.
///
bool CertificateStore::storeServerCertificate(const QByteArray &certificate, const QByteArray &privateKey,
                                              QString *error)
{
    const QSslCertificate parsed = parseCertificate(certificate);
    if (parsed.isNull()) {
        setError(error, tr("The data is not a PEM or DER certificate"));
        return false;
    }
    if (privateKey.isEmpty()) {
        setError(error, tr("The server certificate has no private key"));
        return false;
    }
    const QString previous = serverCertificatePath();
    if (!writeFile(previous, parsed.toDer(), error) || !writeFile(keyPath(), privateKey, error))
        return false;
    const QString target = generatedCertificatePath();
    if (previous != target && !QFile::rename(previous, target)) {
        setError(error, tr("Cannot rename '%1' to '%2'").arg(previous, target));
        return false;
    }
    return true;
}

///
/// \brief Tells whether a server certificate and its private key are installed.
/// \return True when both files exist.
///
bool CertificateStore::hasServerCertificate() const
{
    return QFileInfo::exists(serverCertificatePath()) && QFileInfo::exists(keyPath());
}

///
/// \brief Moves a stored certificate to another folder, e.g. to trust a rejected client.
/// \param filePath Stored certificate file.
/// \param category Target folder.
/// \param error Receives the reason on failure; may be null.
/// \return Path of the moved file, or empty when it is not in the store or cannot be moved.
///
QString CertificateStore::move(const QString &filePath, CertificateInfo::Category category, QString *error)
{
    if (!contains(filePath) || category == CertificateInfo::Category::Application) {
        setError(error, tr("'%1' cannot be moved there").arg(filePath));
        return {};
    }
    const QString target = folder(category) + QLatin1Char('/') + QFileInfo(filePath).fileName();
    QDir().mkpath(folder(category));
    QFile::remove(target);
    if (!QFile::rename(filePath, target)) {
        setError(error, tr("Cannot move '%1'").arg(filePath));
        return {};
    }
    return target;
}

///
/// \brief Deletes a stored certificate; deleting the server certificate also deletes its key.
/// \param filePath Stored certificate file.
/// \param error Receives the reason on failure; may be null.
/// \return False when the file is not in the store or cannot be deleted.
///
bool CertificateStore::remove(const QString &filePath, QString *error)
{
    if (!contains(filePath) || !QFile::remove(filePath)) {
        setError(error, tr("Cannot delete '%1'").arg(filePath));
        return false;
    }
    if (QFileInfo(filePath).absolutePath() == QFileInfo(folder(CertificateInfo::Category::Application)).absoluteFilePath())
        QFile::remove(keyPath());
    return true;
}

///
/// \brief Reads what QUaServer needs for secure endpoints.
/// \return Server certificate and key with the trusted and issuer certificates.
///
PkiMaterial CertificateStore::material() const
{
    PkiMaterial material;
    material.certificate = readFile(serverCertificatePath());
    material.privateKey = readFile(keyPath());
    for (const QFileInfo &file : QDir(folder(CertificateInfo::Category::Trusted)).entryInfoList(QDir::Files))
        material.trusted.append(readFile(file.absoluteFilePath()));
    for (const QFileInfo &file : QDir(folder(CertificateInfo::Category::Issuer)).entryInfoList(QDir::Files))
        material.issuers.append(readFile(file.absoluteFilePath()));
    return material;
}

///
/// \brief Builds the server certificate path from the local hostname.
/// \return Path named OpenUaSim@HOSTNAME.der in the own folder.
///
QString CertificateStore::generatedCertificatePath() const
{
    return folder(CertificateInfo::Category::Application)
           + QStringLiteral("/OpenUaSim@%1.der").arg(QHostInfo::localHostName());
}

///
/// \brief Locates the installed certificate, including certificates from another host.
/// \return Path in the own folder.
///
QString CertificateStore::serverCertificatePath() const
{
    const QString target = generatedCertificatePath();
    if (QFileInfo::exists(target))
        return target;
    const QFileInfoList files = QDir(folder(CertificateInfo::Category::Application)).entryInfoList(
        { QStringLiteral("OpenUaSim@*.der") }, QDir::Files, QDir::Name);
    for (const QFileInfo &file : files) {
        if (!parseCertificate(readFile(file.absoluteFilePath())).isNull())
            return file.absoluteFilePath();
    }
    return target;
}

///
/// \brief Returns where the server private key is kept.
/// \return Path below the own folder.
///
QString CertificateStore::keyPath() const
{
    return _root + QStringLiteral("/own/private/server.key");
}

///
/// \brief Tells whether a file lies in one of the store folders.
/// \param filePath File to check.
/// \return True for an existing file directly inside a folder.
///
bool CertificateStore::contains(const QString &filePath) const
{
    const QFileInfo file(filePath);
    if (!file.isFile())
        return false;
    for (CertificateInfo::Category category : categories()) {
        if (file.absolutePath() == QFileInfo(folder(category)).absoluteFilePath())
            return true;
    }
    return false;
}
