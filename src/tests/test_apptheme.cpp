// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QImage>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

#include "appcolors.h"
#include "appicons.h"
#include "apppalette.h"
#include "apptheme.h"

namespace {

QColor strokeColor(const QIcon &icon)
{
    const QImage image = icon.pixmap(24).toImage();
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() == 255)
                return color;
        }
    }
    return {};
}

}

class TestAppTheme : public QObject
{
    Q_OBJECT

private slots:
    void palettesDifferInBrightness();
    void setModeAppliesAndPersists();
    void setModeEmitsOnlyOnSchemeChange();
    void applySavedModeRestoresPreference();
    void applySavedModeFallsBackToSystem();
    void statusColors();
    void everyIconResourceLoads();
    void unknownIconIsNull();
    void paletteIconsFollowTheTheme();
    void fixedIconsKeepTheirColor();
    void statusDots();
};

void TestAppTheme::palettesDifferInBrightness()
{
    const QPalette dark = AppPalette::build(true);
    const QPalette light = AppPalette::build(false);
    QVERIFY(AppPalette::isDark(dark));
    QVERIFY(!AppPalette::isDark(light));
    QVERIFY(dark.color(QPalette::Base).lightness() < light.color(QPalette::Base).lightness());
    QCOMPARE(dark.color(QPalette::Accent), dark.color(QPalette::Highlight));
    QVERIFY(dark.color(QPalette::Disabled, QPalette::Text) != dark.color(QPalette::Text));
}

void TestAppTheme::setModeAppliesAndPersists()
{
    AppTheme theme;
    theme.setMode(AppTheme::Mode::Dark);
    QCOMPARE(theme.mode(), AppTheme::Mode::Dark);
    QVERIFY(theme.isDark());
    QVERIFY(AppPalette::isDark(QApplication::palette()));
    QCOMPARE(QSettings().value(QStringLiteral("appearance/theme")).toString(), QStringLiteral("Dark"));

    theme.setMode(AppTheme::Mode::Light);
    QVERIFY(!theme.isDark());
    QVERIFY(!AppPalette::isDark(QApplication::palette()));
    QCOMPARE(QSettings().value(QStringLiteral("appearance/theme")).toString(), QStringLiteral("Light"));
}

void TestAppTheme::setModeEmitsOnlyOnSchemeChange()
{
    AppTheme theme;
    theme.setMode(AppTheme::Mode::Light);
    QSignalSpy changed(&theme, &AppTheme::schemeChanged);

    theme.setMode(AppTheme::Mode::Dark);
    theme.setMode(AppTheme::Mode::Dark);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.first().first().toBool(), true);
}

void TestAppTheme::applySavedModeRestoresPreference()
{
    QSettings().setValue(QStringLiteral("appearance/theme"), QStringLiteral("Dark"));
    AppTheme theme;
    theme.applySavedMode();
    QCOMPARE(theme.mode(), AppTheme::Mode::Dark);
    QVERIFY(theme.isDark());
}

void TestAppTheme::applySavedModeFallsBackToSystem()
{
    QSettings().setValue(QStringLiteral("appearance/theme"), QStringLiteral("Purple"));
    AppTheme theme;
    theme.applySavedMode();
    QCOMPARE(theme.mode(), AppTheme::Mode::System);
}

void TestAppTheme::statusColors()
{
    QApplication::setPalette(AppPalette::build(true));
    for (StatusKind kind : { StatusKind::Good, StatusKind::Info, StatusKind::Warning,
                             StatusKind::Error, StatusKind::Inactive })
        QVERIFY(AppColors::status(kind).alpha() > 0);
    QCOMPARE(AppColors::status(StatusKind::None).alpha(), 0);
    QCOMPARE(AppColors::statusText(StatusKind::Good), AppColors::status(StatusKind::Good));
    QCOMPARE(AppColors::statusText(StatusKind::None), QApplication::palette().color(QPalette::WindowText));

    QApplication::setPalette(AppPalette::build(false));
    QVERIFY(AppColors::statusText(StatusKind::Good).lightness()
            < AppColors::status(StatusKind::Good).lightness());
}

void TestAppTheme::everyIconResourceLoads()
{
    QDirIterator it(QStringLiteral(":/icons"), { QStringLiteral("*.svg") });
    int count = 0;
    while (it.hasNext()) {
        const QString name = QFileInfo(it.next()).completeBaseName();
        const QIcon icon = AppIcons::icon(name);
        QVERIFY2(!icon.isNull(), qPrintable(name));
        QVERIFY2(strokeColor(icon).isValid(), qPrintable(name));
        ++count;
    }
    QVERIFY(count > 20);
}

void TestAppTheme::unknownIconIsNull()
{
    QVERIFY(AppIcons::icon(QStringLiteral("does-not-exist")).isNull());
    QVERIFY(AppIcons::icon(QStringLiteral("does-not-exist"), Qt::red).isNull());
}

void TestAppTheme::paletteIconsFollowTheTheme()
{
    const QIcon icon = AppIcons::icon(QStringLiteral("server"));
    QApplication::setPalette(AppPalette::build(true));
    const QColor onDark = strokeColor(icon);
    QApplication::setPalette(AppPalette::build(false));
    const QColor onLight = strokeColor(icon);

    QVERIFY(onDark.isValid());
    QVERIFY(onLight.isValid());
    QVERIFY(onDark.lightness() > onLight.lightness());
}

void TestAppTheme::fixedIconsKeepTheirColor()
{
    const QIcon icon = AppIcons::icon(QStringLiteral("server"), QColor(0x22c55e));
    QApplication::setPalette(AppPalette::build(true));
    const QColor onDark = strokeColor(icon);
    QApplication::setPalette(AppPalette::build(false));
    QVERIFY(onDark.isValid());
    QCOMPARE(strokeColor(icon), onDark);
    QCOMPARE(onDark, QColor(0x22c55e));
}

void TestAppTheme::statusDots()
{
    QVERIFY(AppIcons::statusDot(StatusKind::None).isNull());
    const QIcon error = AppIcons::statusDot(StatusKind::Error);
    QVERIFY(!error.isNull());
    QCOMPARE(error.pixmap(24).toImage().pixelColor(12, 12), AppColors::status(StatusKind::Error));
}

QTEST_MAIN(TestAppTheme)
#include "test_apptheme.moc"
