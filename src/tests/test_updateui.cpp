// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QAction>
#include <QDesktopServices>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTest>
#include <QUrl>

#include "applanguage.h"
#include "mainwindow.h"
#include "messagedialog.h"
#include "updatechecker.h"

class TestUpdateUi : public QObject
{
    Q_OBJECT

private slots:
    void reportsResults_data();
    void reportsResults();
    void opensDownloadOnlyAfterAcceptance();
    void translatesAction();
    void recordUrl(const QUrl &url);

private:
    QUrl _openedUrl;
};

///
/// \brief Provides successful and failed check outcomes.
///
void TestUpdateUi::reportsResults_data()
{
    QTest::addColumn<bool>("failure");
    QTest::newRow("up-to-date") << false;
    QTest::newRow("network-error") << true;
}

///
/// \brief Restores the Help action and presents the check outcome.
///
void TestUpdateUi::reportsResults()
{
    QFETCH(bool, failure);
    MainWindow window;
    auto *action = window.findChild<QAction *>(QStringLiteral("actionCheckForUpdates"));
    auto *checker = window.findChild<UpdateChecker *>();
    QVERIFY(action);
    QVERIFY(checker);
    QVERIFY(action->isEnabled());
    QVERIFY(window.findChild<QMenu *>(QStringLiteral("menuHelp"))->actions().contains(action));
    QVERIFY(window.findChildren<MessageDialog *>().isEmpty());

    emit checker->checkStarted();
    QVERIFY(!action->isEnabled());
    if (failure)
        emit checker->checkFailed(QStringLiteral("network unavailable"));
    else
        emit checker->noUpdatesAvailable();
    QVERIFY(action->isEnabled());
    auto *dialog = window.findChild<MessageDialog *>();
    QVERIFY(dialog);
    QVERIFY(dialog->isVisible());
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Check for Updates"));
    QCOMPARE(dialog->findChild<QLabel *>(QStringLiteral("textLabel"))->text(),
             failure ? QStringLiteral("Failed to check for updates.")
                     : QStringLiteral("You are running the latest version."));
    if (failure)
        QCOMPARE(dialog->findChild<QLabel *>(QStringLiteral("detailsLabel"))->text(),
                 QStringLiteral("network unavailable"));
    dialog->accept();
}

///
/// \brief Captures release navigation without launching a browser.
///
void TestUpdateUi::recordUrl(const QUrl &url)
{
    _openedUrl = url;
}

///
/// \brief Requires accepting the update offer before opening its release page.
///
void TestUpdateUi::opensDownloadOnlyAfterAcceptance()
{
    MainWindow window;
    auto *checker = window.findChild<UpdateChecker *>();
    auto *action = window.findChild<QAction *>(QStringLiteral("actionCheckForUpdates"));
    const QUrl url(QStringLiteral("https://example.test/new-release"));
    QDesktopServices::setUrlHandler(QStringLiteral("https"), this, "recordUrl");
    emit checker->checkStarted();
    emit checker->newVersionAvailable(QStringLiteral("9999.0.0-b2"), url.toString());
    QVERIFY(action->isEnabled());
    auto *dialog = window.findChild<MessageDialog *>();
    QVERIFY(dialog);
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Update Available"));
    QVERIFY(dialog->findChild<QLabel *>(QStringLiteral("textLabel"))->text().contains(QStringLiteral("9999.0.0-b2")));
    QVERIFY(_openedUrl.isEmpty());
    dialog->findChild<QPushButton *>(QStringLiteral("cancelButton"))->click();
    QTRY_VERIFY(window.findChildren<MessageDialog *>().isEmpty());
    QVERIFY(_openedUrl.isEmpty());

    emit checker->newVersionAvailable(QStringLiteral("9999.0.0-b2"), url.toString());
    window.findChild<MessageDialog *>()->findChild<QPushButton *>(QStringLiteral("actionButton"))->click();
    QCOMPARE(_openedUrl, url);
    QDesktopServices::unsetUrlHandler(QStringLiteral("https"));
}

///
/// \brief Updates the Help action when the application language changes.
///
void TestUpdateUi::translatesAction()
{
    MainWindow window;
    auto *action = window.findChild<QAction *>(QStringLiteral("actionCheckForUpdates"));
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::Russian);
    QCoreApplication::processEvents();
    QCOMPARE(action->text(), QStringLiteral("Проверить обновления"));
    AppLanguage::apply(*QCoreApplication::instance(), AppLanguage::Language::English);
}

QTEST_MAIN(TestUpdateUi)
#include "test_updateui.moc"
