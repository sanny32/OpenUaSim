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

signals:
    ///
    /// \brief Asks to import a certificate into the selected folder.
    /// \param category Selected store folder.
    ///
    void importRequested(CertificateInfo::Category category);

    ///
    /// \brief Asks to create a self-signed server certificate.
    ///
    void generateRequested();

    ///
    /// \brief Asks to save a copy of the selected certificate.
    /// \param filePath Stored certificate file.
    ///
    void exportRequested(const QString &filePath);

    ///
    /// \brief Asks to move the selected certificate, i.e. to trust or reject it.
    /// \param filePath Stored certificate file.
    /// \param category Target folder.
    ///
    void moveRequested(const QString &filePath, CertificateInfo::Category category);

    ///
    /// \brief Asks to delete the selected certificate.
    /// \param filePath Stored certificate file.
    ///
    void deleteRequested(const QString &filePath);

private:
    void showCategory(const QModelIndex &current);
    void showCertificate(const QModelIndex &current);
    void showDetailValue(const QModelIndex &index);
    void clearDetails();
    int currentCategory() const;
    QString currentFilePath() const;
    void requestImport();
    void requestExport();
    void requestTrust();
    void requestReject();
    void requestDelete();
    void openContainingFolder();

    Ui::CertificatesPage *ui;
    TreeModel *_categoriesModel;
    CertificatesModel *_certificatesModel;
    QList<CertificateInfo> _certificates;
};
