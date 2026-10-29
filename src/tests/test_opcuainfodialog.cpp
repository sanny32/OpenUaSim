// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTest>
#include <open62541/config.h>

#include "applanguage.h"
#include "dependency_versions.h"
#include "apppalette.h"
#include "mainwindow.h"
#include "opcuainfodialog.h"
#include "server/listenersettings.h"

class TestOpcUaInfoDialog : public QObject
{
    Q_OBJECT

private slots:
    void showsBuildCapabilities();
    void copiesDisplayedInformation();
    void opensFromHelpAndCloses();
    void followsLanguageAndPalette_data();
    void followsLanguageAndPalette();
};

///
/// \brief Reports the linked server stack and capabilities enabled in this build.
///
void TestOpcUaInfoDialog::showsBuildCapabilities()
{
    OpcUaInfoDialog dialog;
    QCOMPARE(dialog.findChild<QLabel *>("quaServerVersionValue")->text(),
             QStringLiteral(OUASIM_QUASERVER_REVISION).isEmpty() ? QStringLiteral("Not available")
                                                               : QStringLiteral(OUASIM_QUASERVER_REVISION));
    QCOMPARE(dialog.findChild<QLabel *>("sdkValue")->text(), QStringLiteral("open62541"));
    QCOMPARE(dialog.findChild<QLabel *>("sdkVersionValue")->text(), QStringLiteral(UA_OPEN62541_VERSION));
    QCOMPARE(dialog.findChild<QLabel *>("roleValue")->text(), QStringLiteral("Server"));
    QCOMPARE(dialog.findChild<QLabel *>("transportValue")->text(), QStringLiteral("opc.tcp"));
    QVERIFY(dialog.findChild<QLabel *>("qtVersionValue")->text().contains(QString::fromLatin1(qVersion())));
#ifdef OUASIM_HAVE_ENCRYPTION
    QCOMPARE(dialog.findChild<QLabel *>("securityPoliciesValue")->text(),
             ListenerSettingsBuilder::securityPolicyNames().join(QStringLiteral(", ")));
    QCOMPARE(dialog.findChild<QLabel *>("securityModesValue")->text(),
             ListenerSettingsBuilder::securityModeNames().join(QStringLiteral(", ")));
    QVERIFY(dialog.findChild<QLabel *>("userTokenTypesValue")->text().contains(QStringLiteral("X509 Certificate")));
#else
    QCOMPARE(dialog.findChild<QLabel *>("securityPoliciesValue")->text(), QStringLiteral("None"));
    QCOMPARE(dialog.findChild<QLabel *>("securityModesValue")->text(), QStringLiteral("None"));
    QVERIFY(!dialog.findChild<QLabel *>("userTokenTypesValue")->text().contains(QStringLiteral("X509")));
#endif
    for (const char *name : {"foundationLinkLabel", "specificationLinkLabel"}) {
        auto *label = dialog.findChild<QLabel *>(name);
        QVERIFY(label->openExternalLinks());
        QVERIFY(label->text().contains(QStringLiteral("https://")));
    }
}

///
/// \brief Copies the same build and security information exposed by the labels.
///
void TestOpcUaInfoDialog::copiesDisplayedInformation()
{
    OpcUaInfoDialog dialog;
    dialog.findChild<QPushButton *>("copyButton")->click();
    QCOMPARE(QApplication::clipboard()->text(), dialog.copyText());
    for (const char *name : {"sdkValue", "sdkVersionValue", "qtVersionValue", "quaServerVersionValue", "securityPoliciesValue",
                             "securityModesValue", "userTokenTypesValue", "roleValue"})
        QVERIFY(dialog.copyText().contains(dialog.findChild<QLabel *>(name)->text()));
    QVERIFY(dialog.copyText().contains(QStringLiteral("https://reference.opcfoundation.org")));
}

///
/// \brief Opens the dialog asynchronously from Help and accepts the Close button.
///
void TestOpcUaInfoDialog::opensFromHelpAndCloses()
{
    MainWindow window;
    auto *action = window.findChild<QAction *>("actionOpcUaInfo");
    QVERIFY(action);
    action->trigger();
    auto *dialog = window.findChild<OpcUaInfoDialog *>();
    QVERIFY(dialog);
    QVERIFY(dialog->isVisible());
    QSignalSpy accepted(dialog, &QDialog::accepted);
    dialog->findChild<QPushButton *>("closeButton")->click();
    QCOMPARE(accepted.size(), 1);
    QTRY_VERIFY(window.findChildren<OpcUaInfoDialog *>().isEmpty());
}

///
/// \brief Covers both languages, palettes and enlarged text.
///
void TestOpcUaInfoDialog::followsLanguageAndPalette_data()
{
    QTest::addColumn<bool>("russian");
    QTest::addColumn<bool>("dark");
    QTest::newRow("english-light") << false << false;
    QTest::newRow("english-dark") << false << true;
    QTest::newRow("russian-light") << true << false;
    QTest::newRow("russian-dark") << true << true;
}

///
/// \brief Retranslates runtime values and keeps long security rows within the scroll area.
///
void TestOpcUaInfoDialog::followsLanguageAndPalette()
{
    QFETCH(bool, russian);
    QFETCH(bool, dark);
    OpcUaInfoDialog dialog;
    QFont font = dialog.font();
    font.setPointSize(12);
    dialog.setFont(font);
    QPalette palette = dialog.palette();
    palette.setColor(QPalette::Window, dark ? QColor(32, 32, 32) : QColor(245, 245, 245));
    palette.setColor(QPalette::WindowText, dark ? Qt::white : Qt::black);
    dialog.setPalette(palette);
    const QImage logo = dialog.findChild<QLabel *>("logoLabel")->pixmap().toImage();
    QVERIFY(!logo.isNull());
    QCOMPARE(logo, QPixmap(dark ? QStringLiteral(":/branding/opcua-dark.png")
                               : QStringLiteral(":/branding/opcua-light.png")).toImage());
    AppLanguage::apply(*qApp, russian ? AppLanguage::Language::Russian : AppLanguage::Language::English);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QCOMPARE(dialog.windowTitle(), russian ? QStringLiteral("Сведения об OPC UA") : QStringLiteral("OPC UA Info"));
    QCOMPARE(dialog.findChild<QLabel *>("roleValue")->text(), russian ? QStringLiteral("Сервер") : QStringLiteral("Server"));
    const auto *scroll = dialog.findChild<QScrollArea *>("scrollArea");
    QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
    QVERIFY(dialog.findChild<QPushButton *>("closeButton")->geometry().width() > 0);
    AppLanguage::apply(*qApp, AppLanguage::Language::English);
}

QTEST_MAIN(TestOpcUaInfoDialog)
#include "test_opcuainfodialog.moc"
