// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatesmodel.h
/// \brief Declares the table model of PKI store certificates.
///

#pragma once

#include "data/certificateinfo.h"
#include "data/statuskind.h"
#include "itemtablemodel.h"

///
/// \brief Table of certificates: common name, validity, store status and fingerprint.
///
class CertificatesModel : public ItemTableModel<CertificateInfo>
{
    Q_OBJECT

public:
    ///
    /// \brief Columns exposed by the model.
    ///
    enum Column { ColCommonName, ColValidFrom, ColValidTo, ColStatus, ColFingerprint, ColCount };

    ///
    /// \brief Constructs an empty model.
    /// \param parent Owning QObject.
    ///
    explicit CertificatesModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns cell text, the certificate icon, the status indicator, and the full fingerprint as a tooltip.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Requested data role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the status text of a store folder.
    /// \param category Store folder.
    /// \return "Own", "Trusted", "Rejected" or "Issuer".
    ///
    static QString statusText(CertificateInfo::Category category);

    ///
    /// \brief Returns the indicator state of a store folder.
    /// \param category Store folder.
    /// \return Error for rejected certificates, Info for issuers, Good otherwise.
    ///
    static StatusKind status(CertificateInfo::Category category);
};
