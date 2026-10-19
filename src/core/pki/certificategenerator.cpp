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
#include <memory>
#include <openssl/rand.h>
#include <openssl/x509.h>
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
// ub-common-name of RFC 5280; OpenSSL refuses longer subject values.
const int kMaxSubjectValueLength = 64;

///
/// \brief Builds a subject entry such as "CN=value", cutting the value to the X.509 length limit.
/// \param field Attribute name.
/// \param value Attribute value; must not be empty.
/// \return UTF-8 subject entry.
///
QByteArray subjectEntry(const char *field, const QString &value)
{
    return QByteArray(field) + '=' + value.left(kMaxSubjectValueLength).toUtf8();
}

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

///
/// \brief Re-signs the generated certificate with a random positive serial number.
/// \param generated Certificate and private key from open62541.
/// \return Certificate and key; both empty on failure.
///
GeneratedCertificate randomizeSerial(GeneratedCertificate generated)
{
    const unsigned char *certificateData = reinterpret_cast<const unsigned char *>(generated.certificate.constData());
    const unsigned char *keyData = reinterpret_cast<const unsigned char *>(generated.privateKey.constData());
    const std::unique_ptr<X509, decltype(&X509_free)> certificate(
        d2i_X509(nullptr, &certificateData, long(generated.certificate.size())), X509_free);
    const std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(
        d2i_AutoPrivateKey(nullptr, &keyData, long(generated.privateKey.size())), EVP_PKEY_free);
    unsigned char bytes[20];
    if (!certificate || !key || RAND_bytes(bytes, sizeof(bytes)) != 1)
        return {};
    bytes[0] = (bytes[0] & 0x7f) | 0x01;
    const std::unique_ptr<BIGNUM, decltype(&BN_free)> number(BN_bin2bn(bytes, sizeof(bytes), nullptr), BN_free);
    if (!number)
        return {};
    const std::unique_ptr<ASN1_INTEGER, decltype(&ASN1_INTEGER_free)> serial(
        BN_to_ASN1_INTEGER(number.get(), nullptr), ASN1_INTEGER_free);
    // open62541's OpenSSL generator hardcodes serial 1 and exposes no serial parameter.
    if (!serial || X509_set_serialNumber(certificate.get(), serial.get()) != 1
        || X509_sign(certificate.get(), key.get(), EVP_sha256()) <= 0)
        return {};
    const int size = i2d_X509(certificate.get(), nullptr);
    if (size <= 0)
        return {};
    generated.certificate.resize(size);
    unsigned char *output = reinterpret_cast<unsigned char *>(generated.certificate.data());
    if (i2d_X509(certificate.get(), &output) != size)
        return {};
    return generated;
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
        subjectEntry("CN", QStringLiteral("%1@%2").arg(QCoreApplication::applicationName(), machine)),
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
    GeneratedCertificate generated = randomizeSerial({ takeBytes(certificate), takeBytes(key) });
    if (generated.certificate.isEmpty())
        setError(error, tr("Cannot assign a random serial number to the server certificate"));
    return generated;
#else
    Q_UNUSED(applicationUri)
    Q_UNUSED(hostnames)
    setError(error, tr("Creating certificates needs a build with encryption support"));
    return {};
#endif
}

}
