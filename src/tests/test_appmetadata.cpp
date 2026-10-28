// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QFile>
#include <QXmlStreamReader>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class TestAppMetadata : public QObject
{
    Q_OBJECT

private slots:
    void executableMetadata_data();
    void executableMetadata();
    void bundleLanguages();
};

///
/// \brief Covers the executable identity exposed to the Windows shell.
///
void TestAppMetadata::executableMetadata_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<QString>("expected");
    QTest::newRow("task-manager-name") << QStringLiteral("FileDescription") << QStringLiteral("OpenUaSim");
    QTest::newRow("product-name") << QStringLiteral("ProductName") << QStringLiteral("OpenUaSim");
    QTest::newRow("file-version") << QStringLiteral("FileVersion") << QStringLiteral(APP_VERSION);
    QTest::newRow("product-version") << QStringLiteral("ProductVersion") << QStringLiteral(APP_VERSION);
    QTest::newRow("original-filename") << QStringLiteral("OriginalFilename") << QStringLiteral("ouasim.exe");
}

///
/// \brief Reads the built executable to catch missing or incorrectly linked version resources.
///
void TestAppMetadata::executableMetadata()
{
#ifdef Q_OS_WIN
    QFETCH(QString, field);
    QFETCH(QString, expected);
    const QString path = QStringLiteral(OUASIM_EXECUTABLE_PATH);
    const auto nativePath = reinterpret_cast<LPCWSTR>(path.utf16());
    const DWORD size = GetFileVersionInfoSizeW(nativePath, nullptr);
    QVERIFY2(size > 0, "Executable has no version resource");
    QByteArray data(size, '\0');
    QVERIFY(GetFileVersionInfoW(nativePath, 0, size, data.data()));

    struct Translation { WORD language; WORD codePage; };
    Translation *translation = nullptr;
    UINT length = 0;
    QVERIFY(VerQueryValueW(data.constData(), L"\\VarFileInfo\\Translation",
                          reinterpret_cast<void **>(&translation), &length));
    QVERIFY(length >= sizeof(Translation));
    const QString query = QStringLiteral("\\StringFileInfo\\%1%2\\%3")
                              .arg(translation->language, 4, 16, QLatin1Char('0'))
                              .arg(translation->codePage, 4, 16, QLatin1Char('0'))
                              .arg(field);
    wchar_t *value = nullptr;
    QVERIFY(VerQueryValueW(data.constData(), reinterpret_cast<LPCWSTR>(query.utf16()),
                          reinterpret_cast<void **>(&value), &length));
    QVERIFY(length > 0);
    QCOMPARE(QString::fromWCharArray(value, length - 1), expected);
#else
    QSKIP("Windows executable resources are only available on Windows");
#endif
}

///
/// \brief Checks that macOS can select every shipped interface language.
///
void TestAppMetadata::bundleLanguages()
{
#ifdef Q_OS_MACOS
    QFile plist(QStringLiteral(OUASIM_BUNDLE_PLIST_PATH));
    QVERIFY2(plist.open(QIODevice::ReadOnly), qPrintable(plist.errorString()));
    QXmlStreamReader xml(&plist);
    QStringList languages;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QLatin1String("key"))
            continue;
        if (xml.readElementText() != QLatin1String("CFBundleLocalizations"))
            continue;
        QVERIFY(xml.readNextStartElement());
        QCOMPARE(xml.name(), QLatin1String("array"));
        while (xml.readNextStartElement()) {
            QCOMPARE(xml.name(), QLatin1String("string"));
            languages.append(xml.readElementText());
        }
    }
    QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
    QCOMPARE(languages, QStringList({QStringLiteral("en"), QStringLiteral("ru")}));
#else
    QSKIP("macOS bundle metadata is only available on macOS");
#endif
}

QTEST_GUILESS_MAIN(TestAppMetadata)
#include "test_appmetadata.moc"
