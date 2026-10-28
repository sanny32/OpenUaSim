// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include "recentdirectories.h"
#include "settingsstore.h"

using RecentDirectories::Purpose;

class TestRecentDirectories : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void emptyWhenNeverRemembered();
    void remembersDirectoryOfFile();
    void purposesAreIndependent();
    void emptyFileNameKeepsDirectory();
    void removedDirectoryIsForgotten();
};

void TestRecentDirectories::init()
{
    SettingsStore().clear();
}

void TestRecentDirectories::emptyWhenNeverRemembered()
{
    QVERIFY(RecentDirectories::directory(Purpose::Project).isEmpty());
    QVERIFY(RecentDirectories::directory(Purpose::Model).isEmpty());
}

void TestRecentDirectories::remembersDirectoryOfFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    RecentDirectories::remember(Purpose::Project, directory.filePath(QStringLiteral("plant.ouasim")));
    QCOMPARE(RecentDirectories::directory(Purpose::Project), QFileInfo(directory.path()).absoluteFilePath());
}

void TestRecentDirectories::purposesAreIndependent()
{
    QTemporaryDir projects;
    QTemporaryDir models;
    QVERIFY(projects.isValid() && models.isValid());
    RecentDirectories::remember(Purpose::Project, projects.filePath(QStringLiteral("plant.ouasim")));
    RecentDirectories::remember(Purpose::Model, models.filePath(QStringLiteral("model.xml")));
    QCOMPARE(RecentDirectories::directory(Purpose::Project), QFileInfo(projects.path()).absoluteFilePath());
    QCOMPARE(RecentDirectories::directory(Purpose::Model), QFileInfo(models.path()).absoluteFilePath());
}

void TestRecentDirectories::emptyFileNameKeepsDirectory()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    RecentDirectories::remember(Purpose::Model, directory.filePath(QStringLiteral("model.xml")));
    RecentDirectories::remember(Purpose::Model, QString());
    QCOMPARE(RecentDirectories::directory(Purpose::Model), QFileInfo(directory.path()).absoluteFilePath());
}

void TestRecentDirectories::removedDirectoryIsForgotten()
{
    QString removed;
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        removed = directory.filePath(QStringLiteral("model.xml"));
    }
    RecentDirectories::remember(Purpose::Model, removed);
    QVERIFY(RecentDirectories::directory(Purpose::Model).isEmpty());
}

QTEST_GUILESS_MAIN(TestRecentDirectories)

#include "test_recentdirectories.moc"
