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
    /// \brief Key usage text.
    QString keyUsage;
    /// \brief Location of the certificate file.
    QString filePath;
};
