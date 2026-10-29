// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file aboutdialog.cpp
/// \brief Implements the application information window.
///

#include <QDate>
#include <QEvent>
#include <QIcon>
#include <QLocale>
#include <QSysInfo>

#include "aboutdialog.h"
#include "appicons.h"
#include "apppalette.h"
#include "ui_aboutdialog.h"

///
/// \brief Presents product information and build diagnostics.
/// \param parent Owning widget.
///
AboutDialog::AboutDialog(QWidget *parent)
    : AppBaseDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    const QIcon logo(QStringLiteral(":/branding/logo.svg"));
    setWindowIcon(logo);
    ui->logoLabel->setPixmap(logo.pixmap(QSize(76, 76), devicePixelRatioF()));
    ui->contentArea->setBackgroundRole(QPalette::Base);
    QFont titleFont = font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.9);
    titleFont.setWeight(QFont::Bold);
    ui->nameLabel->setFont(titleFont);
    populateInformation();
    applyColors();
    connect(ui->okButton, &QPushButton::clicked, this, &QDialog::accept);
}

///
/// \brief Releases the generated form.
///
AboutDialog::~AboutDialog()
{
    delete ui;
}

///
/// \brief Refreshes translated metadata and theme colours after application changes.
/// \param event Change event.
///
void AboutDialog::changeEvent(QEvent *event)
{
    AppBaseDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        populateInformation();
        if (isVisible())
            fitToContents();
    }
    if (event->type() == QEvent::PaletteChange)
        applyColors();
}

///
/// \brief Sizes the window for the actual font metrics before it appears.
/// \param event Show event.
///
void AboutDialog::showEvent(QShowEvent *event)
{
    fitToContents();
    AppBaseDialog::showEvent(event);
}

///
/// \brief Prevents spare window height from stretching the footer and metadata rows.
///
void AboutDialog::fitToContents()
{
    layout()->activate();
    const int width = qMax(640, minimumSizeHint().width());
    setFixedSize(width, layout()->totalHeightForWidth(width));
}

///
/// \brief Resolves product metadata from the running build and operating system.
///
void AboutDialog::populateInformation()
{
    setWindowTitle(tr("About %1").arg(QStringLiteral(APP_NAME)));
    ui->nameLabel->setText(QStringLiteral(APP_NAME));
    ui->versionValue->setText(QStringLiteral(APP_VERSION));
    const QDate buildDate = QLocale::c().toDate(QStringLiteral(__DATE__).simplified(), QStringLiteral("MMM d yyyy"));
    ui->buildValue->setText(buildDate.toString(Qt::ISODate) + QStringLiteral(" " __TIME__));
    ui->qtValue->setText(QString::fromLatin1(qVersion()));
    ui->platformValue->setText(tr("%1 (%2) %3-bit")
                                  .arg(QSysInfo::prettyProductName(), QSysInfo::kernelVersion())
                                  .arg(QSysInfo::WordSize));
    ui->copyrightValue->setText(tr("© %1 OpenUaSim contributors.").arg(QStringLiteral(BUILD_YEAR)));
}

///
/// \brief Keeps supporting text and contact icons legible in the active theme.
///
void AboutDialog::applyColors()
{
    const bool dark = AppPalette::isDark(palette());
    QPalette subtitle = palette();
    subtitle.setColor(QPalette::Text, dark ? QColor(0x94a3b8) : QColor(0x60697b));
    ui->subtitleLabel->setPalette(subtitle);
    for (QLabel *label : {ui->versionCaption, ui->buildCaption, ui->qtCaption,
                          ui->platformCaption, ui->copyrightCaption, ui->licenseCaption})
        label->setPalette(subtitle);
    ui->websiteIcon->setPixmap(AppIcons::icon(QStringLiteral("globe")).pixmap(QSize(16, 16), devicePixelRatioF()));
    ui->repositoryIcon->setPixmap(AppIcons::icon(QStringLiteral("github")).pixmap(QSize(16, 16), devicePixelRatioF()));
    ui->emailIcon->setPixmap(AppIcons::icon(QStringLiteral("mail")).pixmap(QSize(16, 16), devicePixelRatioF()));
}
