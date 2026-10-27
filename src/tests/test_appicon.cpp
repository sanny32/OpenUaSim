// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QIcon>
#include <QImage>
#include <QtTest>

#include "application.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

///
/// \brief Measures visible artwork independently of the icon canvas.
///
QRect opaqueBounds(const QImage &image)
{
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() >= 128)
                bounds |= QRect(x, y, 1, 1);
        }
    }
    return bounds;
}

}

class TestAppIcon : public QObject
{
    Q_OBJECT

private slots:
    void windowIcon_data();
    void windowIcon();
    void smallIconKeepsOriginalArtwork();
    void taskbarIconKeepsOriginalGlyph();
    void wheel32UsesMoreOfCanvas();
    void matchesMacArtwork();
    void nativeWindowIconSizes();
    void executableIcon();
};

///
/// \brief Covers shell icon sizes and high-DPI variants without raster upscaling.
///
void TestAppIcon::windowIcon_data()
{
    QTest::addColumn<int>("size");
    QTest::addColumn<qreal>("dpr");
    for (int size : {16, 24, 32, 48, 64, 128, 256}) {
        QTest::newRow(qPrintable(QString::number(size))) << size << qreal(1);
        if (size <= 128)
            QTest::newRow(qPrintable(QString::number(size) + "@2x")) << size << qreal(2);
    }
}

///
/// \brief Keeps the application icon at the requested shell size with limited transparent margins.
///
void TestAppIcon::windowIcon()
{
    QFETCH(int, size);
    QFETCH(qreal, dpr);
    const QIcon icon = QApplication::windowIcon();
    QVERIFY(!icon.isNull());
    const QImage image = icon.pixmap(QSize(size, size), dpr).toImage();
    QCOMPARE(image.size(), QSize(size * dpr, size * dpr));
    const QRect bounds = opaqueBounds(image);
#ifdef Q_OS_WIN
    if (image.width() >= 24) {
        QCOMPARE(bounds, image.rect());
    } else
#endif
    {
        QVERIFY(bounds.width() >= qRound(image.width() * 0.9));
        QVERIFY(bounds.height() >= qRound(image.height() * 0.9));
    }
    QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
}

///
/// \brief Preserves the original glyph and margins in the 16px title-bar icon.
///
void TestAppIcon::smallIconKeepsOriginalArtwork()
{
    const QIcon original(QStringLiteral(":/icons/app.svg"));
    const QImage expected = original.pixmap(QSize(16, 16), 1.0).toImage().convertToFormat(QImage::Format_ARGB32);
    const QImage actual = QApplication::windowIcon().pixmap(QSize(16, 16), 1.0).toImage().convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(actual, expected);
}

///
/// \brief Expands only the 24px background while preserving every pixel of the wheel.
///
void TestAppIcon::taskbarIconKeepsOriginalGlyph()
{
    const QIcon original(QStringLiteral(":/icons/app.svg"));
    const QImage expected = original.pixmap(QSize(24, 24), 1.0).toImage();
    const QImage actual = QApplication::windowIcon().pixmap(QSize(24, 24), 1.0).toImage();
    for (int y = 0; y < actual.height(); ++y) {
        for (int x = 0; x < actual.width(); ++x) {
            const QColor color = expected.pixelColor(x, y);
            if (color.alpha() == 255)
                QCOMPARE(actual.pixelColor(x, y), color);
        }
    }
}

///
/// \brief Keeps the enlarged 32px wheel centred with about five pixels of padding.
///
void TestAppIcon::wheel32UsesMoreOfCanvas()
{
#ifndef Q_OS_WIN
    QSKIP("The 32px wheel is a Windows icon variant");
#endif
    const QImage image = QApplication::windowIcon().pixmap(QSize(32, 32), 1.0).toImage();
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.red() > 146)
                bounds |= QRect(x, y, 1, 1);
        }
    }
    QVERIFY(bounds.width() >= 21 && bounds.width() <= 23);
    QVERIFY(bounds.height() >= 21 && bounds.height() <= 23);
    QCOMPARE(bounds.center(), image.rect().center());
}

///
/// \brief Keeps the shell icon's white symbol consistent with the macOS artwork.
///
void TestAppIcon::matchesMacArtwork()
{
#ifndef Q_OS_WIN
    QSKIP("The large Windows icon uses the macOS artwork");
#endif
    const QImage reference(QStringLiteral(OUASIM_MAC_ICON_PATH));
    QVERIFY(!reference.isNull());
    const QImage expected = reference.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    const QImage actual = QApplication::windowIcon().pixmap(QSize(256, 256), 1.0).toImage();
    QCOMPARE(actual.size(), expected.size());
    int mismatches = 0;
    for (int y = 0; y < actual.height(); ++y) {
        for (int x = 0; x < actual.width(); ++x) {
            const QColor pixel = actual.pixelColor(x, y);
            const bool white = pixel.red() > 230 && pixel.green() > 230 && pixel.blue() > 230;
            if (white != (expected.pixelColor(x, y).alpha() >= 128))
                ++mismatches;
        }
    }
    QVERIFY2(mismatches < actual.width() * actual.height() / 100,
             qPrintable(QStringLiteral("%1 pixels differ from the macOS symbol").arg(mismatches)));
    QCOMPARE(actual.pixelColor(128, 8), QColor(QStringLiteral("#2563eb")));
}

///
/// \brief Exposes native sizes to Windows rather than a single nominal SVG size.
///
void TestAppIcon::nativeWindowIconSizes()
{
#ifdef Q_OS_WIN
    const QList<QSize> sizes = QApplication::windowIcon().availableSizes();
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        QVERIFY2(sizes.contains(QSize(size, size)), qPrintable(QStringLiteral("Missing %1px window icon").arg(size)));
#else
    QSKIP("Native Windows icon sizes are only required on Windows");
#endif
}

///
/// \brief Checks that the built Windows executable embeds every shell icon resolution.
///
void TestAppIcon::executableIcon()
{
#ifdef Q_OS_WIN
    const QString path = QStringLiteral(OUASIM_EXECUTABLE_PATH);
    const HMODULE module = LoadLibraryExW(reinterpret_cast<LPCWSTR>(path.utf16()), nullptr,
                                         LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    QVERIFY(module);
    const HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(1), MAKEINTRESOURCEW(14));
    QByteArray group;
    if (resource) {
        const HGLOBAL data = LoadResource(module, resource);
        if (data)
            group = QByteArray(static_cast<const char *>(LockResource(data)), SizeofResource(module, resource));
    }
    QList<QImage> nativeImages;
    for (int size : {24, 32}) {
        const HICON nativeIcon = static_cast<HICON>(LoadImageW(module, MAKEINTRESOURCEW(1), IMAGE_ICON, size, size, 0));
        nativeImages.append(QImage::fromHICON(nativeIcon));
        if (nativeIcon)
            DestroyIcon(nativeIcon);
    }
    FreeLibrary(module);
    QCOMPARE(nativeImages[0].size(), QSize(24, 24));
    QCOMPARE(nativeImages[1].size(), QSize(32, 32));
    for (const QImage &image : nativeImages)
        QCOMPARE(opaqueBounds(image), image.rect());
    QVERIFY2(!group.isEmpty(), "Executable has no application icon resource");
    QVERIFY(group.size() >= 6);
    const int count = quint8(group[4]) | (quint8(group[5]) << 8);
    QCOMPARE(group.size(), 6 + count * 14);
    QList<int> sizes;
    for (int i = 0; i < count; ++i) {
        const int offset = 6 + i * 14;
        QCOMPARE(group[offset], group[offset + 1]);
        sizes.append(group[offset] ? quint8(group[offset]) : 256);
        QCOMPARE(quint8(group[offset + 6]), 32);
    }
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        QVERIFY2(sizes.contains(size), qPrintable(QStringLiteral("Missing %1px icon").arg(size)));
#else
    QSKIP("Windows executable resources are only available on Windows");
#endif
}

///
/// \brief Exercises the window icon installed by the production application.
///
int main(int argc, char **argv)
{
    Application app(argc, argv);
    TestAppIcon test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_appicon.moc"
