// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QImage>
#include <QSignalSpy>
#include <QTest>

#include "appcolors.h"
#include "appicons.h"
#include "apppalette.h"
#include "apptheme.h"
#include "settingsstore.h"

namespace {

QColor strokeColor(const QIcon &icon, QIcon::Mode mode = QIcon::Normal)
{
    const QImage image = icon.pixmap(QSize(24, 24), mode).toImage();
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
    void paletteIconsFollowTheTheme_data();
    void paletteIconsFollowTheTheme();
    void accentIconsKeepTheirColour();
    void variableIconsRenderAsGreyPng();
    void statusDots();
    void statusIconsGreyOutWhenDisabled();
    void serverControlIconsGreyOutWhenDisabled_data();
    void serverControlIconsGreyOutWhenDisabled();
    void messageIcons_data();
    void messageIcons();
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
    QCOMPARE(SettingsStore().value(QStringLiteral("appearance/theme")).toString(), QStringLiteral("Dark"));

    theme.setMode(AppTheme::Mode::Light);
    QVERIFY(!theme.isDark());
    QVERIFY(!AppPalette::isDark(QApplication::palette()));
    QCOMPARE(SettingsStore().value(QStringLiteral("appearance/theme")).toString(), QStringLiteral("Light"));
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
    SettingsStore().setValue(QStringLiteral("appearance/theme"), QStringLiteral("Dark"));
    AppTheme theme;
    theme.applySavedMode();
    QCOMPARE(theme.mode(), AppTheme::Mode::Dark);
    QVERIFY(theme.isDark());
}

void TestAppTheme::applySavedModeFallsBackToSystem()
{
    SettingsStore().setValue(QStringLiteral("appearance/theme"), QStringLiteral("Purple"));
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
    QVERIFY(count > 15);
}

void TestAppTheme::unknownIconIsNull()
{
    QVERIFY(AppIcons::icon(QStringLiteral("does-not-exist")).isNull());
}

void TestAppTheme::paletteIconsFollowTheTheme_data()
{
    QTest::addColumn<QString>("name");
    QTest::newRow("save") << QStringLiteral("save");
    QTest::newRow("reference-type") << QStringLiteral("reference-type");
    QTest::newRow("data-type") << QStringLiteral("data-type");
    for (const AppIcons::NamedIcon &icon : AppIcons::variableIcons()) {
        if (!icon.name.isEmpty() && !icon.name.startsWith(QStringLiteral("status-")))
            QTest::newRow(qPrintable(icon.name)) << icon.name;
    }
}

void TestAppTheme::paletteIconsFollowTheTheme()
{
    QFETCH(QString, name);
    const QIcon icon = AppIcons::icon(name);
    QApplication::setPalette(AppPalette::build(true));
    const QColor onDark = strokeColor(icon);
    QApplication::setPalette(AppPalette::build(false));
    const QColor onLight = strokeColor(icon);

    QVERIFY(onDark.isValid());
    QVERIFY(onLight.isValid());
    QVERIFY(onDark.lightness() > onLight.lightness());
}

void TestAppTheme::accentIconsKeepTheirColour()
{
    const QColor accent(0x0d9488);
    const QIcon icon = AppIcons::icon(QStringLiteral("variable-type"));
    QApplication::setPalette(AppPalette::build(false));
    QCOMPARE(strokeColor(icon), accent);
    QCOMPARE(strokeColor(icon, QIcon::Selected), QApplication::palette().color(QPalette::HighlightedText));
    QCOMPARE(strokeColor(icon, QIcon::Disabled),
             QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));

    QApplication::setPalette(AppPalette::build(true));
    QVERIFY(strokeColor(icon).lightness() > accent.lightness());
    QCOMPARE(strokeColor(icon).hue(), accent.hue());
}

void TestAppTheme::variableIconsRenderAsGreyPng()
{
    QApplication::setPalette(AppPalette::build(true));
    for (const AppIcons::NamedIcon &icon : AppIcons::variableIcons()) {
        const QByteArray png = AppIcons::imagePng(icon.name);
        if (icon.name.isEmpty()) {
            QVERIFY(png.isEmpty());
            continue;
        }
        QVERIFY2(png.startsWith("\x89PNG"), qPrintable(icon.name));
        const QImage image = QImage::fromData(png, "PNG");
        QCOMPARE(image.size(), QSize(32, 32));
        const bool monochrome = !icon.name.startsWith(QStringLiteral("status-"));
        bool drawn = false;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor color = image.pixelColor(x, y);
                if (color.alpha() == 255) {
                    if (monochrome)
                        QCOMPARE(color, QColor(0x808080));
                    drawn = true;
                }
            }
        }
        QVERIFY2(drawn, qPrintable(icon.name));
    }
    QVERIFY(AppIcons::imagePng(QStringLiteral("does-not-exist")).isEmpty());
}

void TestAppTheme::statusDots()
{
    QVERIFY(AppIcons::statusDot(StatusKind::None).isNull());
    const QIcon error = AppIcons::statusDot(StatusKind::Error);
    QVERIFY(!error.isNull());
    QCOMPARE(error.pixmap(24).toImage().pixelColor(12, 12), AppColors::status(StatusKind::Error));
}

void TestAppTheme::statusIconsGreyOutWhenDisabled()
{
    QVERIFY(AppIcons::statusIcon(QStringLiteral("does-not-exist"), StatusKind::Good).isNull());
    const QIcon icon = AppIcons::statusIcon(QStringLiteral("shield-check"), StatusKind::Good);
    QCOMPARE(strokeColor(icon), AppColors::status(StatusKind::Good));
    QCOMPARE(strokeColor(icon, QIcon::Disabled),
             QGuiApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
}

void TestAppTheme::serverControlIconsGreyOutWhenDisabled_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QColor>("accent");
    QTest::newRow("play") << QStringLiteral("play") << QColor(0x22c55e);
    QTest::newRow("stop") << QStringLiteral("stop") << QColor(0xef4444);
}

void TestAppTheme::serverControlIconsGreyOutWhenDisabled()
{
    QFETCH(QString, name);
    QFETCH(QColor, accent);
    QApplication::setPalette(AppPalette::build(false));
    const QIcon icon = AppIcons::icon(name);
    QCOMPARE(strokeColor(icon), accent);
    QCOMPARE(strokeColor(icon, QIcon::Disabled),
             QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
}

void TestAppTheme::messageIcons_data()
{
    QTest::addColumn<QStyle::StandardPixmap>("pixmap");
    QTest::addColumn<StatusKind>("kind");
    QTest::newRow("information") << QStyle::SP_MessageBoxInformation << StatusKind::Info;
    QTest::newRow("question") << QStyle::SP_MessageBoxQuestion << StatusKind::Info;
    QTest::newRow("warning") << QStyle::SP_MessageBoxWarning << StatusKind::Warning;
    QTest::newRow("critical") << QStyle::SP_MessageBoxCritical << StatusKind::Error;
}

void TestAppTheme::messageIcons()
{
    QFETCH(QStyle::StandardPixmap, pixmap);
    QFETCH(StatusKind, kind);
    const QIcon icon = AppIcons::messageIcon(pixmap);
    QVERIFY(!icon.isNull());
    // Sampled below the glyph, inside both the circle and the warning triangle.
    QCOMPARE(icon.pixmap(24).toImage().pixelColor(6, 19), AppColors::status(kind));
    QVERIFY(AppIcons::messageIcon(QStyle::SP_DirIcon).isNull());
}

QTEST_MAIN(TestAppTheme)
#include "test_apptheme.moc"
