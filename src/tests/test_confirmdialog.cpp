// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTest>

#include "appcolors.h"
#include "apppalette.h"
#include "appstyle.h"
#include "confirmdialog.h"

namespace {

template <typename T>
T *child(const QWidget &parent, const char *name)
{
    T *found = parent.findChild<T *>(QLatin1String(name));
    if (!found)
        qFatal("Missing child %s", name);
    return found;
}

ConfirmDialog *createDialog(const QString &details = QStringLiteral("The client will no longer be trusted."))
{
    return new ConfirmDialog(QStringLiteral("Delete Certificate"), QStringLiteral("Delete the certificate 'A'?"),
                             details, QStringLiteral("Delete"));
}

}

class TestConfirmDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void showsQuestionAndDetails();
    void hidesEmptyDetails();
    void cancelIsTheDefault();
    void actionAccepts();
    void questionStandsOutFromDetails();
    void detailsFollowTheTheme();
    void contentSitsOnBaseAndActionIsFilled();
};

void TestConfirmDialog::init()
{
    QApplication::setPalette(AppPalette::build(false));
}

void TestConfirmDialog::showsQuestionAndDetails()
{
    QScopedPointer<ConfirmDialog> dialog(createDialog());
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Delete Certificate"));
    QCOMPARE(child<QLabel>(*dialog, "questionLabel")->text(), QStringLiteral("Delete the certificate 'A'?"));
    QCOMPARE(child<QLabel>(*dialog, "detailsLabel")->text(), QStringLiteral("The client will no longer be trusted."));
    QCOMPARE(child<QPushButton>(*dialog, "actionButton")->text(), QStringLiteral("Delete"));
    QVERIFY(!child<QLabel>(*dialog, "iconLabel")->pixmap().isNull());
}

void TestConfirmDialog::hidesEmptyDetails()
{
    QScopedPointer<ConfirmDialog> dialog(createDialog(QString()));
    dialog->show();
    QVERIFY(!child<QLabel>(*dialog, "detailsLabel")->isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(dialog.data()));
    QVERIFY2(dialog->width() >= 520, qPrintable(QString::number(dialog->width())));
}

void TestConfirmDialog::cancelIsTheDefault()
{
    QScopedPointer<ConfirmDialog> dialog(createDialog());
    dialog->show();
    QVERIFY(child<QPushButton>(*dialog, "cancelButton")->isDefault());
    QVERIFY(!child<QPushButton>(*dialog, "actionButton")->isDefault());
    QTest::keyClick(dialog.data(), Qt::Key_Return);
    QCOMPARE(dialog->result(), int(QDialog::Rejected));
}

void TestConfirmDialog::actionAccepts()
{
    QScopedPointer<ConfirmDialog> dialog(createDialog());
    dialog->show();
    child<QPushButton>(*dialog, "actionButton")->click();
    QCOMPARE(dialog->result(), int(QDialog::Accepted));
}

void TestConfirmDialog::questionStandsOutFromDetails()
{
    QScopedPointer<ConfirmDialog> dialog(createDialog());
    const QFont question = child<QLabel>(*dialog, "questionLabel")->font();
    const QFont details = child<QLabel>(*dialog, "detailsLabel")->font();
    QCOMPARE(question.weight(), QFont::DemiBold);
    QVERIFY(details.weight() < QFont::DemiBold);
    QVERIFY(question.pointSizeF() > details.pointSizeF());
}

void TestConfirmDialog::detailsFollowTheTheme()
{
    QScopedPointer<ConfirmDialog> dialog(createDialog());
    dialog->show();
    const auto *details = child<QLabel>(*dialog, "detailsLabel");
    const QColor light = details->palette().color(QPalette::WindowText);
    QCOMPARE(light, AppColors::secondaryText(dialog->palette()));

    QApplication::setPalette(AppPalette::build(true));
    QTRY_VERIFY(AppPalette::isDark(dialog->palette()));
    QCOMPARE(details->palette().color(QPalette::WindowText), AppColors::secondaryText(dialog->palette()));
    QVERIFY(details->palette().color(QPalette::WindowText) != light);
}

void TestConfirmDialog::contentSitsOnBaseAndActionIsFilled()
{
    AppStyle style;
    QScopedPointer<ConfirmDialog> dialog(createDialog());
    dialog->setStyle(&style);
    for (QWidget *widget : dialog->findChildren<QWidget *>())
        widget->setStyle(&style);
    dialog->show();
    QVERIFY(QTest::qWaitForWindowExposed(dialog.data()));

    const QImage content = child<QWidget>(*dialog, "contentArea")->grab().toImage();
    QCOMPARE(content.pixelColor(2, 2), dialog->palette().color(QPalette::Base));
    const QImage footer = child<QWidget>(*dialog, "footer")->grab().toImage();
    QCOMPARE(footer.pixelColor(2, 2), dialog->palette().color(QPalette::Window));
    auto *action = child<QPushButton>(*dialog, "actionButton");
    const QImage button = action->grab().toImage();
    const QColor accent = dialog->palette().color(QPalette::Accent);
    const QColor middle = button.pixelColor(4, button.height() / 2);
    QVERIFY(qAbs(middle.hue() - accent.hue()) <= 4);
    QVERIFY(qAbs(middle.lightness() - accent.lightness()) <= 16);
    QVERIFY(button.pixelColor(4, 3).lightness() > button.pixelColor(4, button.height() - 4).lightness());
}

QTEST_MAIN(TestConfirmDialog)
#include "test_confirmdialog.moc"
