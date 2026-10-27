// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatespage.h
/// \brief Declares the certificate store page.
///

#pragma once

#include <QList>
#include <QStringList>
#include <QWidget>

#include "data/certificateinfo.h"

namespace Ui {
class CertificatesPage;
}

class CertificatesModel;
class QAction;
class QDropEvent;
class QMenu;
class QMimeData;
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
    /// \brief Shows the certificates of every store folder, keeping the selected folder and certificate.
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
    /// \brief Asks to import certificate files dropped on the certificate list.
    /// \param fileNames Dropped local files.
    /// \param category Trusted, Rejected or Issuer folder they go to.
    ///
    void filesDropped(const QStringList &fileNames, CertificateInfo::Category category);

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

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void buildImportMenu();
    void applyPalette();
    void showPem();
    void importInto(const QAction *action);
    bool acceptsDrop(const QMimeData *mimeData) const;
    void dropFiles(QDropEvent *event);
    void fitDetailsWidth();
    void showCategory(const QModelIndex &current);
    void showCertificate(const QModelIndex &current);
    void reselectCertificate(const QString &fingerprint, int row);
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
    QMenu *_importMenu;
    QList<CertificateInfo> _certificates;
};
