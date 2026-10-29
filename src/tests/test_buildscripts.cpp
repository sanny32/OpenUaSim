#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QtTest>

class BuildScriptsTest : public QObject
{
    Q_OBJECT

private slots:
    void bash();
    void powershell();
    void packaging();

private:
    void runBashTest(const QString &script);
};

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
