// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificategenerator.cpp
/// \brief Implements the creation of a self-signed server application certificate.
///

#include <QCoreApplication>
#include <QHostInfo>
#include <QList>

#ifdef OUASIM_HAVE_ENCRYPTION
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>
#endif

#include "certificategenerator.h"

namespace {

///
/// \brief Translates a message of the certificate generator.
/// \param text Source text.
/// \return Translated text.
///
QString tr(const char *text)
{
    return QCoreApplication::translate("CertificateGenerator", text);
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

#ifdef OUASIM_HAVE_ENCRYPTION
const UA_UInt16 kKeySizeBits = 2048;
const UA_UInt16 kValidDays = 3650;

///
/// \brief Points UA strings at UTF-8 buffers that must outlive them.
/// \param texts Buffers.
/// \return Strings referencing the buffers.
///
QList<UA_String> uaStrings(const QList<QByteArray> &texts)
{
    QList<UA_String> result;
    for (const QByteArray &text : texts)
        result.append(UA_String { size_t(text.size()), reinterpret_cast<UA_Byte *>(const_cast<char *>(text.data())) });
    return result;
}

///
/// \brief Copies a UA byte string and frees it.
/// \param bytes Bytes owned by open62541.
/// \return Copy.
///
QByteArray takeBytes(UA_ByteString &bytes)
{
    const QByteArray copy(reinterpret_cast<const char *>(bytes.data), qsizetype(bytes.length));
    UA_ByteString_clear(&bytes);
    return copy;
}
#endif

}

namespace CertificateGenerator {

///
/// \brief Tells whether this build can create certificates.
/// \return True in builds with encryption support.
///
bool isAvailable()
{
#ifdef OUASIM_HAVE_ENCRYPTION
    return true;
#else
    return false;
#endif
}

///
/// \brief Creates a self-signed RSA application certificate for the server.
/// \param applicationUri Application URI the certificate names; OPC UA clients compare it with the server's.
/// \param hostnames Host names clients reach the server by; empty entries are skipped.
/// \param error Receives the reason on failure; may be null.
/// \return Certificate and key; both empty on failure.
///
GeneratedCertificate generate(const QString &applicationUri, const QStringList &hostnames, QString *error)
{
#ifdef OUASIM_HAVE_ENCRYPTION
    const QString machine = QHostInfo::localHostName();
    const QList<QByteArray> subjectTexts = {
        QStringLiteral("CN=%1@%2").arg(QCoreApplication::applicationName(), machine).toUtf8(),
        QStringLiteral("O=%1").arg(QCoreApplication::organizationName()).toUtf8(),
    };
    QList<QByteArray> altNameTexts = { QStringLiteral("URI:%1").arg(applicationUri).toUtf8() };
    QStringList dnsNames = hostnames;
    dnsNames.append(machine);
    dnsNames.removeAll(QString());
    dnsNames.removeDuplicates();
    for (const QString &name : std::as_const(dnsNames))
        altNameTexts.append(QStringLiteral("DNS:%1").arg(name).toUtf8());

    QList<UA_String> subject = uaStrings(subjectTexts);
    QList<UA_String> altNames = uaStrings(altNameTexts);
    UA_KeyValueMap *params = UA_KeyValueMap_new();
    UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, const_cast<char *>("key-size-bits")),
                             const_cast<UA_UInt16 *>(&kKeySizeBits), &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, const_cast<char *>("expires-in-days")),
                             const_cast<UA_UInt16 *>(&kValidDays), &UA_TYPES[UA_TYPES_UINT16]);
    UA_ByteString key = UA_BYTESTRING_NULL;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    const UA_StatusCode status = UA_CreateCertificate(UA_Log_Stdout, subject.data(), size_t(subject.size()),
                                                      altNames.data(), size_t(altNames.size()),
                                                      UA_CERTIFICATEFORMAT_DER, params, &key, &certificate);
    UA_KeyValueMap_delete(params);
    if (status != UA_STATUSCODE_GOOD) {
        setError(error, tr("Cannot create a server certificate: %1").arg(QString::fromLatin1(UA_StatusCode_name(status))));
        return {};
    }
    return { takeBytes(certificate), takeBytes(key) };
#else
    Q_UNUSED(applicationUri)
    Q_UNUSED(hostnames)
    setError(error, tr("Creating certificates needs a build with encryption support"));
    return {};
#endif
}

}
