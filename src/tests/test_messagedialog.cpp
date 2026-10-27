// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTest>

#include "appcolors.h"
#include "apppalette.h"
#include "appstyle.h"
#include "messagedialog.h"

namespace {

template <typename T>
T *child(const QWidget &parent, const char *name)
{
    T *found = parent.findChild<T *>(QLatin1String(name));
    if (!found)
        qFatal("Missing child %s", name);
    return found;
}

MessageDialog *createConfirmation(const QString &details = QStringLiteral("The client will no longer be trusted."))
{
    return MessageDialog::confirmation(QStringLiteral("Delete Certificate"), QStringLiteral("Delete the certificate 'A'?"),
                                       details, QStringLiteral("Delete"));
}

}

class TestMessageDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void noticeHasOnlyOk();
    void confirmationShowsQuestionAndDetails();
    void hidesEmptyDetails();
    void confirmationDefaultsToCancel();
    void actionAccepts();
    void alternativeFinishesWithItsOwnResult();
    void actionCanBeTheDefault();
    void choiceNamesBothButtons();
    void textStandsOutFromDetails();
    void detailsFollowTheTheme();
    void contentSitsOnBaseAndActionIsFilled();
    void fitsShortTextWithoutEmptySpace();
    void wrapsLongTextAtTheMaximumWidth();
};

void TestMessageDialog::fitsShortTextWithoutEmptySpace()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation());
    dialog->show();
    QVERIFY(QTest::qWaitForWindowExposed(dialog.data()));
    QCOMPARE(dialog->height(), dialog->layout()->totalHeightForWidth(dialog->width()));
    QCOMPARE(dialog->minimumSize(), dialog->maximumSize());
    const auto *text = child<QLabel>(*dialog, "textLabel");
    QVERIFY(text->height() < 2 * text->fontMetrics().lineSpacing());
    const auto *details = child<QLabel>(*dialog, "detailsLabel");
    const auto *content = child<QWidget>(*dialog, "contentArea");
    QCOMPARE(content->height() - details->geometry().bottom() - 1, content->layout()->contentsMargins().bottom());
}

void TestMessageDialog::wrapsLongTextAtTheMaximumWidth()
{
    const QString longText = QStringLiteral("Cannot open the file ").repeated(12);
    MessageDialog dialog(QStyle::SP_MessageBoxWarning, QStringLiteral("Error"), longText);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QCOMPARE(dialog.width(), 720);
    QCOMPARE(dialog.height(), dialog.layout()->totalHeightForWidth(dialog.width()));
    const auto *text = child<QLabel>(dialog, "textLabel");
    QVERIFY(text->height() > 2 * text->fontMetrics().lineSpacing());
}

void TestMessageDialog::init()
{
    QApplication::setPalette(AppPalette::build(false));
}

void TestMessageDialog::noticeHasOnlyOk()
{
    MessageDialog dialog(QStyle::SP_MessageBoxWarning, QStringLiteral("Log not exported"),
                         QStringLiteral("Cannot write the file."));
    dialog.show();
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Log not exported"));
    QCOMPARE(child<QLabel>(dialog, "textLabel")->text(), QStringLiteral("Cannot write the file."));
    QVERIFY(!child<QLabel>(dialog, "detailsLabel")->isVisible());
    QVERIFY(!child<QPushButton>(dialog, "cancelButton")->isVisible());
    QVERIFY(!child<QPushButton>(dialog, "alternativeButton")->isVisible());
    QCOMPARE(child<QPushButton>(dialog, "actionButton")->text(), QStringLiteral("OK"));
    QTest::keyClick(&dialog, Qt::Key_Return);
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
}

void TestMessageDialog::confirmationShowsQuestionAndDetails()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation());
    dialog->show();
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Delete Certificate"));
    QCOMPARE(child<QLabel>(*dialog, "textLabel")->text(), QStringLiteral("Delete the certificate 'A'?"));
    QCOMPARE(child<QLabel>(*dialog, "detailsLabel")->text(), QStringLiteral("The client will no longer be trusted."));
    QCOMPARE(child<QPushButton>(*dialog, "actionButton")->text(), QStringLiteral("Delete"));
    QVERIFY(child<QPushButton>(*dialog, "cancelButton")->isVisible());
    QVERIFY(!child<QPushButton>(*dialog, "alternativeButton")->isVisible());
    QVERIFY(!child<QLabel>(*dialog, "iconLabel")->pixmap().isNull());
}

void TestMessageDialog::hidesEmptyDetails()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation(QString()));
    dialog->show();
    QVERIFY(!child<QLabel>(*dialog, "detailsLabel")->isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(dialog.data()));
    QVERIFY2(dialog->width() >= 520, qPrintable(QString::number(dialog->width())));
}

void TestMessageDialog::confirmationDefaultsToCancel()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation());
    dialog->show();
    QVERIFY(child<QPushButton>(*dialog, "cancelButton")->isDefault());
    QVERIFY(!child<QPushButton>(*dialog, "actionButton")->isDefault());
    QTest::keyClick(dialog.data(), Qt::Key_Return);
    QCOMPARE(dialog->result(), int(QDialog::Rejected));
}

void TestMessageDialog::actionAccepts()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation());
    dialog->show();
    child<QPushButton>(*dialog, "actionButton")->click();
    QCOMPARE(dialog->result(), int(QDialog::Accepted));
}

void TestMessageDialog::alternativeFinishesWithItsOwnResult()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation());
    dialog->setAlternative(QStringLiteral("Don't Save"));
    dialog->show();
    auto *alternative = child<QPushButton>(*dialog, "alternativeButton");
    QVERIFY(alternative->isVisible());
    QCOMPARE(alternative->text(), QStringLiteral("Don't Save"));
    alternative->click();
    QCOMPARE(dialog->result(), MessageDialog::Alternative);
}

void TestMessageDialog::actionCanBeTheDefault()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation());
    dialog->setActionDefault();
    dialog->show();
    QVERIFY(child<QPushButton>(*dialog, "actionButton")->isDefault());
    QVERIFY(!child<QPushButton>(*dialog, "cancelButton")->isDefault());
    QTest::keyClick(dialog.data(), Qt::Key_Return);
    QCOMPARE(dialog->result(), int(QDialog::Accepted));
}

void TestMessageDialog::choiceNamesBothButtons()
{
    MessageDialog dialog(QStyle::SP_MessageBoxQuestion, QStringLiteral("Restart Required"),
                         QStringLiteral("Restart the server now?"));
    dialog.setChoice(QStringLiteral("Restart Now"), QStringLiteral("Later"));
    dialog.show();
    auto *cancel = child<QPushButton>(dialog, "cancelButton");
    QCOMPARE(child<QPushButton>(dialog, "actionButton")->text(), QStringLiteral("Restart Now"));
    QCOMPARE(cancel->text(), QStringLiteral("Later"));
    QVERIFY(cancel->isVisible());
    QVERIFY(child<QPushButton>(dialog, "actionButton")->isDefault());
    cancel->click();
    QCOMPARE(dialog.result(), int(QDialog::Rejected));
}

void TestMessageDialog::textStandsOutFromDetails()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation());
    const QFont text = child<QLabel>(*dialog, "textLabel")->font();
    const QFont details = child<QLabel>(*dialog, "detailsLabel")->font();
    QCOMPARE(text.weight(), QFont::DemiBold);
    QVERIFY(details.weight() < QFont::DemiBold);
    QVERIFY(text.pointSizeF() > details.pointSizeF());
}

void TestMessageDialog::detailsFollowTheTheme()
{
    QScopedPointer<MessageDialog> dialog(createConfirmation());
    dialog->show();
    const auto *details = child<QLabel>(*dialog, "detailsLabel");
    const QColor light = details->palette().color(QPalette::WindowText);
    QCOMPARE(light, AppColors::secondaryText(dialog->palette()));

    QApplication::setPalette(AppPalette::build(true));
    QTRY_VERIFY(AppPalette::isDark(dialog->palette()));
    QCOMPARE(details->palette().color(QPalette::WindowText), AppColors::secondaryText(dialog->palette()));
    QVERIFY(details->palette().color(QPalette::WindowText) != light);
}

void TestMessageDialog::contentSitsOnBaseAndActionIsFilled()
{
    AppStyle style;
    QScopedPointer<MessageDialog> dialog(createConfirmation());
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

QTEST_MAIN(TestMessageDialog)
#include "test_messagedialog.moc"
