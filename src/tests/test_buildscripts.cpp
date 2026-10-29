#include <QFileInfo>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>

class BuildScriptsTest : public QObject
{
    Q_OBJECT

private slots:
    void dependencyVersion();
    void bash();
    void powershell();
    void packaging();
    void installerArtwork();
    void installerArtworkHasNoBorder();
    void windowsInstaller();

private:
    void runBashTest(const QString &script);
};

///
/// \brief Resolves dependency versions and commit fallbacks without modifying checkouts.
///
void BuildScriptsTest::dependencyVersion()
{
    const QString cmake = QStandardPaths::findExecutable(QStringLiteral("cmake"));
    if (cmake.isEmpty() || QStandardPaths::findExecutable(QStringLiteral("git")).isEmpty())
        QSKIP("CMake and Git are required");
    if (!QFileInfo::exists(QStringLiteral(OUASIM_SOURCE_DIR "/.git")))
        QSKIP("The source archive has no Git metadata");
    QTemporaryDir fixture;
    QVERIFY(fixture.isValid());
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(cmake, {QStringLiteral("-DOUASIM_SOURCE_ROOT=" OUASIM_SOURCE_DIR),
                          QStringLiteral("-DFIXTURE_ROOT=%1").arg(fixture.path()),
                          QStringLiteral("-P"),
                          QStringLiteral(OUASIM_SOURCE_DIR "/src/tests/buildscripts/test_dependency_version.cmake")});
    QVERIFY(process.waitForStarted());
    QVERIFY(process.waitForFinished(30000));
    const QByteArray output = process.readAll();
    QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, output.constData());
}

///
/// \brief Keeps the welcome illustration compatible with NSIS bitmap loading.
///
void BuildScriptsTest::installerArtwork()
{
    QFile bitmap(QStringLiteral(OUASIM_SOURCE_DIR "/.github/windows/installer-sidebar.bmp"));
    QVERIFY(bitmap.open(QIODevice::ReadOnly));
    const QByteArray header = bitmap.read(54);
    QCOMPARE(header.size(), 54);
    QCOMPARE(header.left(2), QByteArray("BM"));
    QCOMPARE(qFromLittleEndian<qint32>(header.constData() + 18), 164);
    QCOMPARE(qFromLittleEndian<qint32>(header.constData() + 22), 314);
    QCOMPARE(qFromLittleEndian<quint16>(header.constData() + 28), 24);
    QCOMPARE(qFromLittleEndian<quint32>(header.constData() + 30), 0u);
    QVERIFY(bitmap.size() >= 54 + 164 * 314 * 3);
}

///
/// \brief Rejects dark edge artefacts introduced by bitmap resampling.
///
void BuildScriptsTest::installerArtworkHasNoBorder()
{
    QFile bitmap(QStringLiteral(OUASIM_SOURCE_DIR "/.github/windows/installer-sidebar.bmp"));
    QVERIFY(bitmap.open(QIODevice::ReadOnly));
    const QByteArray pixels = bitmap.readAll();
    QVERIFY(pixels.size() >= 54);
    const quint32 offset = qFromLittleEndian<quint32>(pixels.constData() + 10);
    constexpr int width = 164;
    constexpr int height = 314;
    constexpr int stride = (width * 3 + 3) & ~3;
    QVERIFY(pixels.size() >= offset + stride * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x != 0 && x != width - 1 && y != 0 && y != height - 1)
                continue;
            const int adjacentX = qBound(1, x, width - 2);
            const int adjacentY = qBound(1, y, height - 2);
            for (int channel = 0; channel < 3; ++channel) {
                const auto edge = static_cast<quint8>(pixels[offset + y * stride + x * 3 + channel]);
                const auto adjacent = static_cast<quint8>(pixels[offset + adjacentY * stride + adjacentX * 3 + channel]);
                QVERIFY2(qAbs(int(edge) - int(adjacent)) < 80, "Bitmap edge must not contain a high-contrast frame");
            }
        }
    }
}

///
/// \brief Compiles the actual installer script with a disposable payload and its branded artwork.
///
void BuildScriptsTest::windowsInstaller()
{
#ifndef Q_OS_WIN
    QSKIP("The NSIS installer requires Windows");
#endif
    QString executable = QStandardPaths::findExecutable(QStringLiteral("makensis"));
    if (executable.isEmpty()) {
        const QString installed = qEnvironmentVariable("ProgramFiles(x86)") + QStringLiteral("/NSIS/makensis.exe");
        if (QFileInfo::exists(installed))
            executable = installed;
    }
    if (executable.isEmpty())
        QSKIP("NSIS is unavailable");
    QTemporaryDir fixture;
    QVERIFY(fixture.isValid());
    for (const auto &name : {"ouasim.exe", "vc_redist.x64.exe"}) {
        QFile payload(fixture.filePath(QString::fromLatin1(name)));
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QVERIFY(payload.write("test payload") > 0);
    }
    const QString installer = fixture.filePath(QStringLiteral("setup.exe"));
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(executable, {QStringLiteral("/V2"),
                              QStringLiteral("/DSRC_DIR=" OUASIM_SOURCE_DIR),
                              QStringLiteral("/DSTAGE_DIR=%1").arg(fixture.path()),
                              QStringLiteral("/DAPP_VERSION=1.0.0"), QStringLiteral("/DFILE_VERSION=1.0.0.0"),
                              QStringLiteral("/DOUT_FILE=%1").arg(installer),
                              QStringLiteral(OUASIM_SOURCE_DIR "/.github/windows/ouasim.nsi")});
    QVERIFY(process.waitForStarted());
    QVERIFY(process.waitForFinished(60000));
    const QByteArray output = process.readAll();
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QVERIFY2(process.exitCode() == 0, output.constData());
    QVERIFY(QFileInfo(installer).size() > 0);
}

///
/// \brief Exercises Bash dependency selection and build orchestration without installing tools.
///
void BuildScriptsTest::bash()
{
    runBashTest(QStringLiteral("src/tests/buildscripts/test_build.sh"));
}

///
/// \brief Checks package versions and rejection of incompatible Linux binaries.
///
void BuildScriptsTest::packaging()
{
    runBashTest(QStringLiteral("src/tests/buildscripts/test_packaging.sh"));
}

///
/// \brief Runs a dependency-free shell suite using native Bash on each platform.
/// \param script Suite path relative to the repository root.
///
void BuildScriptsTest::runBashTest(const QString &script)
{
    QString executable;
#ifdef Q_OS_WIN
    // The Windows system bash.exe launches WSL, which cannot use our Windows paths and tools.
    const QString gitBash = qEnvironmentVariable("ProgramFiles") + QStringLiteral("/Git/bin/bash.exe");
    if (QFileInfo::exists(gitBash))
        executable = gitBash;
#endif
    if (executable.isEmpty())
        executable = QStandardPaths::findExecutable(QStringLiteral("bash"));
    if (executable.isEmpty())
        QSKIP("Bash is unavailable");

    QProcess process;
    process.setWorkingDirectory(QStringLiteral(OUASIM_SOURCE_DIR));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(executable, {script});
    QVERIFY(process.waitForStarted());
    QVERIFY(process.waitForFinished(60000));
    const QByteArray output = process.readAll();
    QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, output.constData());
}

///
/// \brief Exercises PowerShell argument validation and Qt selection without installing tools.
///
void BuildScriptsTest::powershell()
{
#ifndef Q_OS_WIN
    QSKIP("The PowerShell build script requires Windows");
#endif
    QString executable = QStandardPaths::findExecutable(QStringLiteral("pwsh"));
    if (executable.isEmpty())
        executable = QStandardPaths::findExecutable(QStringLiteral("powershell"));
    if (executable.isEmpty())
        QSKIP("PowerShell is unavailable");

    QProcess process;
    process.setWorkingDirectory(QStringLiteral(OUASIM_SOURCE_DIR));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(executable, {QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                              QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                              QStringLiteral("-File"), QStringLiteral("src/tests/buildscripts/test_build.ps1")});
    QVERIFY(process.waitForStarted());
    QVERIFY(process.waitForFinished(60000));
    const QByteArray output = process.readAll();
    QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, output.constData());
}

QTEST_GUILESS_MAIN(BuildScriptsTest)
#include "test_buildscripts.moc"
