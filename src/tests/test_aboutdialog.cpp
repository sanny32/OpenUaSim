// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QSysInfo>
#include <QTest>

#include "aboutdialog.h"
#include "applanguage.h"
#include "apppalette.h"

namespace {

///
/// \brief Measures whether rendered text contains the requested colour despite antialiasing.
/// \param label Label whose rendered pixels are inspected.
/// \param expected Intended foreground colour.
/// \return Smallest maximum channel difference from the intended colour.
///
int renderedColorDistance(QLabel *label, const QColor &expected)
{
    const QImage image = label->grab().toImage();
    int closest = 255;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            const int difference = qMax(qAbs(pixel.red() - expected.red()),
                                       qMax(qAbs(pixel.green() - expected.green()), qAbs(pixel.blue() - expected.blue())));
            closest = qMin(closest, difference);
        }
    }
    return closest;
}

}

class TestAboutDialog : public QObject
{
    Q_OBJECT

private slots:
    void metadataAndLinks();
    void acceptsAndRejects();
    void followsTheme();
    void translatedLayout();
    void proportionsFollowContent_data();
    void proportionsFollowContent();
};

///
/// \brief Exercises compact and enlarged application fonts in both supported languages.
///
void TestAboutDialog::proportionsFollowContent_data()
{
    QTest::addColumn<int>("pointSize");
    QTest::addColumn<bool>("russian");
    QTest::newRow("small-english") << 9 << false;
    QTest::newRow("large-english") << 12 << false;
    QTest::newRow("small-russian") << 9 << true;
    QTest::newRow("large-russian") << 12 << true;
}

///
/// \brief Preserves the application font and prevents excess space around the footer and description.
///
void TestAboutDialog::proportionsFollowContent()
{
    QFETCH(int, pointSize);
    QFETCH(bool, russian);
    AppLanguage::apply(*qApp, russian ? AppLanguage::Language::Russian : AppLanguage::Language::English);
    const QFont original = QApplication::font();
    QFont applicationFont = original;
    applicationFont.setPointSize(pointSize);
    QApplication::setFont(applicationFont);
    AboutDialog dialog;
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    for (const auto *label : dialog.findChildren<QLabel *>()) {
        if (label->objectName() != QStringLiteral("nameLabel"))
            QCOMPARE(label->font(), applicationFont);
    }
    QCOMPARE(dialog.height(), dialog.layout()->totalHeightForWidth(dialog.width()));
    const auto *footer = dialog.findChild<QWidget *>("footer");
    const auto *button = dialog.findChild<QPushButton *>("okButton");
    const QMargins margins = footer->layout()->contentsMargins();
    QCOMPARE(footer->height(), button->height() + margins.top() + margins.bottom());
    const auto *description = dialog.findChild<QLabel *>("descriptionLabel");
    QVERIFY(description->height() >= description->heightForWidth(description->width()));
    QVERIFY(description->height() <= description->heightForWidth(description->width()) + 1);
    const auto *title = dialog.findChild<QLabel *>("nameLabel");
    auto *version = dialog.findChild<QLabel *>("versionValue");
    QVERIFY(version->mapTo(&dialog, QPoint()).x() >= title->mapTo(&dialog, QPoint()).x());
    const auto *firstContact = dialog.findChild<QLabel *>("websiteIcon");
    const auto *secondContact = dialog.findChild<QLabel *>("repositoryIcon");
    QCOMPARE(secondContact->y() - firstContact->y(), firstContact->height() + 4);
    QApplication::setFont(original);
    AppLanguage::apply(*qApp, AppLanguage::Language::English);
}

///
/// \brief Verifies runtime diagnostics and keyboard-accessible contact links.
///
void TestAboutDialog::metadataAndLinks()
{
    AboutDialog dialog;
    QCOMPARE(dialog.windowTitle(), QStringLiteral("About " APP_NAME));
    QCOMPARE(dialog.findChild<QLabel *>("nameLabel")->text(), QStringLiteral(APP_NAME));
    QCOMPARE(dialog.findChild<QLabel *>("versionValue")->text(), QStringLiteral(APP_VERSION));
    QVERIFY(!dialog.findChild<QLabel *>("versionLabel"));
    QCOMPARE(dialog.findChild<QLabel *>("qtValue")->text(), QString::fromLatin1(qVersion()));
    QVERIFY(dialog.findChild<QLabel *>("platformValue")->text().contains(QSysInfo::prettyProductName()));
    QVERIFY(QRegularExpression(QStringLiteral("^\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}$"))
                .match(dialog.findChild<QLabel *>("buildValue")->text()).hasMatch());
    QVERIFY(!dialog.findChild<QLabel *>("logoLabel")->pixmap().isNull());
    const QStringList names = {"websiteLink", "repositoryLink", "emailLink", "licenseValue"};
    const QStringList urls = {"https://sanny32.github.io/OpenUaSim", "https://github.com/sanny32/OpenUaSim",
                              "mailto:mail@ananev.org", "https://github.com/sanny32/OpenUaSim/blob/main/LICENSE"};
    for (int i = 0; i < names.size(); ++i) {
        auto *label = dialog.findChild<QLabel *>(names[i]);
        QVERIFY(label->openExternalLinks());
        QVERIFY(label->textInteractionFlags().testFlag(Qt::LinksAccessibleByKeyboard));
        QVERIFY(label->text().contains(QStringLiteral("href=\"%1\"").arg(urls[i])));
        if (i < 3) {
            const QString address = urls[i].startsWith(QStringLiteral("mailto:")) ? urls[i].mid(7) : urls[i];
            QVERIFY(label->text().contains(QStringLiteral(">%1</a>").arg(address)));
        }
    }
}

///
/// \brief Preserves standard dialog acceptance and cancellation shortcuts.
///
void TestAboutDialog::acceptsAndRejects()
{
    AboutDialog dialog;
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    QSignalSpy rejected(&dialog, &QDialog::rejected);
    dialog.show();
    QTest::keyClick(&dialog, Qt::Key_Return);
    QCOMPARE(accepted.count(), 1);
    dialog.show();
    QTest::keyClick(&dialog, Qt::Key_Escape);
    QCOMPARE(rejected.count(), 1);
    dialog.show();
    dialog.findChild<QPushButton *>("okButton")->click();
    QCOMPARE(accepted.count(), 2);
}

///
/// \brief Checks supporting text after a live palette switch.
///
void TestAboutDialog::followsTheme()
{
    QApplication::setPalette(AppPalette::build(false));
    AboutDialog dialog;
    dialog.show();
    auto *subtitle = dialog.findChild<QLabel *>("subtitleLabel");
    QCOMPARE(subtitle->palette().color(subtitle->foregroundRole()), QColor(0x60697b));
    QVERIFY(renderedColorDistance(subtitle, QColor(0x60697b)) <= 20);
    const QStringList captions = {"versionCaption", "buildCaption", "qtCaption",
                                   "platformCaption", "copyrightCaption", "licenseCaption"};
    for (const auto &name : captions) {
        auto *label = dialog.findChild<QLabel *>(name);
        QCOMPARE(label->palette().color(label->foregroundRole()), QColor(0x60697b));
        QVERIFY2(renderedColorDistance(label, QColor(0x60697b)) <= 20, qPrintable(name));
    }
    QApplication::setPalette(AppPalette::build(true));
    QTRY_VERIFY(AppPalette::isDark(dialog.palette()));
    QCOMPARE(subtitle->palette().color(subtitle->foregroundRole()), QColor(0x94a3b8));
    QVERIFY(renderedColorDistance(subtitle, QColor(0x94a3b8)) <= 20);
    for (const auto &name : captions) {
        auto *label = dialog.findChild<QLabel *>(name);
        QCOMPARE(label->palette().color(label->foregroundRole()), QColor(0x94a3b8));
        QVERIFY2(renderedColorDistance(label, QColor(0x94a3b8)) <= 20, qPrintable(name));
    }
    QCOMPARE(dialog.findChild<QWidget *>("contentArea")->backgroundRole(), QPalette::Base);
    QApplication::setPalette(AppPalette::build(false));
}

///
/// \brief Keeps translated content within the layout and restores English dynamically.
///
void TestAboutDialog::translatedLayout()
{
    AboutDialog dialog;
    QVERIFY(AppLanguage::apply(*qApp, AppLanguage::Language::Russian));
    QTRY_COMPARE(dialog.windowTitle(), QString::fromUtf8("О программе OpenUaSim"));
    QCOMPARE(dialog.findChild<QLabel *>("versionCaption")->text(), QString::fromUtf8("Версия"));
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    for (const auto *label : dialog.findChildren<QLabel *>()) {
        QVERIFY2(dialog.rect().contains(label->mapTo(&dialog, QPoint(0, 0))), qPrintable(label->objectName()));
        QVERIFY2(label->width() >= label->minimumSizeHint().width(), qPrintable(label->objectName()));
        if (label->hasHeightForWidth())
            QVERIFY2(label->height() >= label->heightForWidth(label->width()), qPrintable(label->objectName()));
    }
    AppLanguage::apply(*qApp, AppLanguage::Language::English);
    QTRY_COMPARE(dialog.windowTitle(), QStringLiteral("About " APP_NAME));
}

QTEST_MAIN(TestAboutDialog)
#include "test_aboutdialog.moc"
