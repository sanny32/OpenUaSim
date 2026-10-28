// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

#include "addnodedialog.h"
#include "alarmdialog.h"
#include "messagedialog.h"
#include "renamenodedialog.h"
#include "serversettingsdialog.h"
#include "subscriptionsdialog.h"
#include "usersdialog.h"
#include "valuedialog.h"

#include <type_traits>

static_assert(std::is_base_of_v<AppBaseDialog, AddNodeDialog>);
static_assert(std::is_base_of_v<AppBaseDialog, AlarmDialog>);
static_assert(std::is_base_of_v<AppBaseDialog, MessageDialog>);
static_assert(std::is_base_of_v<AppBaseDialog, RenameNodeDialog>);
static_assert(std::is_base_of_v<AppBaseDialog, ServerSettingsDialog>);
static_assert(std::is_base_of_v<AppBaseDialog, SubscriptionsDialog>);
static_assert(std::is_base_of_v<AppBaseDialog, UsersDialog>);
static_assert(std::is_base_of_v<AppBaseDialog, ValueDialog>);

#ifdef Q_OS_MAC
bool isTitledMacDialog(QWidget *widget);
#endif

class TestAppBaseDialog : public QObject
{
    Q_OBJECT

private slots:
    void opensAsynchronously_data();
    void opensAsynchronously();
    void execStillWorks();
    void renameInput();
};

///
/// \brief Covers ownership, acceptance and cancellation across repeated openings.
///
void TestAppBaseDialog::opensAsynchronously_data()
{
    QTest::addColumn<bool>("hasParent");
    QTest::addColumn<bool>("accept");
    QTest::newRow("parent-accept") << true << true;
    QTest::newRow("parent-cancel") << true << false;
    QTest::newRow("no-parent-accept") << false << true;
    QTest::newRow("no-parent-cancel") << false << false;
}

///
/// \brief Checks modal presentation and completion without entering a nested event loop.
///
void TestAppBaseDialog::opensAsynchronously()
{
    QFETCH(bool, hasParent);
    QFETCH(bool, accept);
    QWidget parent;
    parent.show();
    ServerSettingsDialog dialog(ServerSettings {}, hasParent ? &parent : nullptr);
    QSignalSpy finished(&dialog, &QDialog::finished);
    for (int attempt = 0; attempt < 2; ++attempt) {
        QDialog *base = &dialog;
        base->open();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        QVERIFY(dialog.isModal());
        QVERIFY(!dialog.windowTitle().isEmpty());
        QCOMPARE(dialog.parentWidget(), hasParent ? &parent : nullptr);
#ifdef Q_OS_MAC
        QCOMPARE(dialog.windowModality(), Qt::ApplicationModal);
        QVERIFY(dialog.windowType() != Qt::Sheet);
        if (QApplication::platformName() == QLatin1String("cocoa"))
            QVERIFY(isTitledMacDialog(&dialog));
#else
        QCOMPARE(dialog.windowModality(), Qt::WindowModal);
#endif
        if (accept)
            dialog.accept();
        else
            QTest::keyClick(&dialog, Qt::Key_Escape);
        QVERIFY(!dialog.isVisible());
        QCOMPARE(finished.count(), attempt + 1);
        QCOMPARE(dialog.result(), int(accept ? QDialog::Accepted : QDialog::Rejected));
    }
}

///
/// \brief Preserves synchronous callers of the shared base.
///
void TestAppBaseDialog::execStillWorks()
{
    AppBaseDialog dialog;
    QTimer::singleShot(0, &dialog, &QDialog::accept);
    QCOMPARE(dialog.exec(), int(QDialog::Accepted));
}

///
/// \brief Keeps the current name selected and prevents an empty rename.
///
void TestAppBaseDialog::renameInput()
{
    RenameNodeDialog dialog(QStringLiteral("Motor"));
    auto *edit = dialog.findChild<QLineEdit *>(QStringLiteral("nameEdit"));
    auto *buttons = dialog.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(edit);
    QVERIFY(buttons);
    QCOMPARE(edit->selectedText(), QStringLiteral("Motor"));
    edit->clear();
    QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
    edit->setText(QStringLiteral("Pump"));
    QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
    dialog.open();
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    buttons->button(QDialogButtonBox::Ok)->click();
    QCOMPARE(accepted.count(), 1);
    QCOMPARE(dialog.browseName(), QStringLiteral("Pump"));
}

QTEST_MAIN(TestAppBaseDialog)
#include "test_appbasedialog.moc"
