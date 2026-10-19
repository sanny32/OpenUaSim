// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificategenerator.h
/// \brief Declares the creation of a self-signed server application certificate.
///

#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

///
/// \brief Self-signed certificate with its private key, both DER.
///
struct GeneratedCertificate
{
    /// \brief DER certificate.
    QByteArray certificate;
    /// \brief DER private key.
    QByteArray privateKey;
};

namespace CertificateGenerator {

///
/// \brief Tells whether this build can create certificates.
/// \return True in builds with encryption support.
///
bool isAvailable();

///
/// \brief Creates a self-signed RSA application certificate for the server.
/// \param applicationUri Application URI the certificate names; OPC UA clients compare it with the server's.
/// \param hostnames Host names clients reach the server by; empty entries are skipped.
/// \param error Receives the reason on failure; may be null.
/// \return Certificate and key; both empty on failure.
///
GeneratedCertificate generate(const QString &applicationUri, const QStringList &hostnames, QString *error = nullptr);

}
