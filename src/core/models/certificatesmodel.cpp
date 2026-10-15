// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatesmodel.cpp
/// \brief Implements the table model of PKI store certificates.
///

#include "certificatesmodel.h"
#include "itemroles.h"

namespace {

///
/// \brief Formats a validity bound to the minute, as the certificate table shows it.
/// \param time Timestamp to format.
/// \return Text such as "2024-04-10 10:00".
///
QString validityText(const QDateTime &time)
{
    return time.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

}

///
/// \brief Constructs an empty model.
/// \param parent Owning QObject.
///
CertificatesModel::CertificatesModel(QObject *parent)
    : ItemTableModel(parent)
{
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int CertificatesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief Returns cell text, the certificate icon, and the status indicator.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant CertificatesModel::data(const QModelIndex &index, int role) const
{
    if (!hasItem(index))
        return {};
    const CertificateInfo &certificate = item(index.row());

    if (role == ItemRoles::Id)
        return certificate.serialNumber;
    if (role == ItemRoles::IconName && index.column() == ColCommonName)
        return QStringLiteral("certificate");
    if (role == ItemRoles::Status && index.column() == ColStatus)
        return ItemRoles::statusValue(status(certificate.category));
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case ColCommonName:  return certificate.commonName;
    case ColValidFrom:   return validityText(certificate.validFrom);
    case ColValidTo:     return validityText(certificate.validTo);
    case ColStatus:      return statusText(certificate.category);
    case ColFingerprint: return certificate.fingerprint;
    default:             return {};
    }
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested data role.
/// \return Column title, or the base implementation otherwise.
///
QVariant CertificatesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return ItemTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColCommonName:  return tr("Common Name");
    case ColValidFrom:   return tr("Valid From");
    case ColValidTo:     return tr("Valid To");
    case ColStatus:      return tr("Status");
    case ColFingerprint: return tr("Fingerprint");
    default:             return {};
    }
}

///
/// \brief Returns the status text of a store folder.
/// \param category Store folder.
/// \return "Own", "Trusted", "Rejected" or "Issuer".
///
QString CertificatesModel::statusText(CertificateInfo::Category category)
{
    switch (category) {
    case CertificateInfo::Category::Application: return tr("Own");
    case CertificateInfo::Category::Trusted:     return tr("Trusted");
    case CertificateInfo::Category::Rejected:    return tr("Rejected");
    case CertificateInfo::Category::Issuer:      return tr("Issuer");
    }
    return {};
}

///
/// \brief Returns the indicator state of a store folder.
/// \param category Store folder.
/// \return Error for rejected certificates, Info for issuers, Good otherwise.
///
StatusKind CertificatesModel::status(CertificateInfo::Category category)
{
    switch (category) {
    case CertificateInfo::Category::Rejected: return StatusKind::Error;
    case CertificateInfo::Category::Issuer:   return StatusKind::Info;
    default:                                  return StatusKind::Good;
    }
}
