// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatespage.h
/// \brief Declares the certificate store page.
///

#pragma once

#include <QList>
#include <QWidget>

#include "data/certificateinfo.h"

namespace Ui {
class CertificatesPage;
}

class CertificatesModel;
class TreeModel;

///
/// \brief Page browsing the PKI store folders, their certificates and certificate details.
///
class CertificatesPage : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the page.
    /// \param parent Parent widget.
    ///
    explicit CertificatesPage(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the page.
    ///
    ~CertificatesPage() override;

    ///
    /// \brief Shows the certificates of every store folder.
    /// \param certificates Certificates to show.
    ///
    void setCertificates(const QList<CertificateInfo> &certificates);

    ///
    /// \brief Returns the table model of the selected folder.
    /// \return Model of the certificates in the selected folder.
    ///
    const CertificatesModel *certificatesModel() const;

private:
    void showCategory(const QModelIndex &current);
    void showCertificate(const QModelIndex &current);

    Ui::CertificatesPage *ui;
    TreeModel *_categoriesModel;
    CertificatesModel *_certificatesModel;
    QList<CertificateInfo> _certificates;
};
