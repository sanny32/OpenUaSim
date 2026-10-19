// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificateinfo.h
/// \brief Declares the description of a certificate in the server PKI store.
///

#pragma once

#include <QDateTime>
#include <QString>

///
/// \brief Describes one certificate of the server PKI store.
///
struct CertificateInfo
{
    ///
    /// \brief Store folder the certificate belongs to.
    ///
    enum class Category { Application, Trusted, Rejected, Issuer };

    /// \brief Store folder.
    Category category = Category::Trusted;
    /// \brief Subject common name.
    QString commonName;
    /// \brief X.509 version.
    QString version;
    /// \brief Algorithm used to sign the certificate.
    QString signatureAlgorithm;
    /// \brief Digest used by the signature algorithm, when applicable.
    QString signatureHashAlgorithm;
    /// \brief Public key algorithm and size.
    QString publicKey;
    /// \brief DER-encoded public key algorithm parameters as spaced hex.
    QString publicKeyParameters;
    /// \brief Certificate authority flag and path length constraints.
    QString basicConstraints;
    /// \brief Purposes allowed by the extended key usage extension.
    QString extendedKeyUsage;
    /// \brief Identifier of the subject's public key.
    QString subjectKeyIdentifier;
    /// \brief Identifier of the issuer's public key.
    QString authorityKeyIdentifier;
    /// \brief Alternative URIs, DNS names, IP addresses and email addresses.
    QString subjectAlternativeNames;
    /// \brief Full subject distinguished name.
    QString subject;
    /// \brief Full issuer distinguished name.
    QString issuer;
    /// \brief Serial number as spaced hex.
    QString serialNumber;
    /// \brief Start of the validity period.
    QDateTime validFrom;
    /// \brief End of the validity period.
    QDateTime validTo;
    /// \brief SHA-1 fingerprint as colon-separated hex.
    QString fingerprint;
    /// \brief SHA-256 fingerprint as colon-separated hex.
    QString sha256Fingerprint;
    /// \brief Decoded X.509 fields and extensions.
    QString details;
    /// \brief Key usage text.
    QString keyUsage;
    /// \brief Location of the certificate file.
    QString filePath;
};
