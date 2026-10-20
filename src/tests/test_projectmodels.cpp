// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include "project/projectmodels.h"

namespace {

bool writeFile(const QString &fileName, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

QByteArray readFile(const QString &fileName)
{
    QFile file(fileName);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

}

class TestProjectModels : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void directoryIsNamedAfterProject();
    void copiesModelsNextToProject();
    void keepsModelsAlreadyThere();
    void renamesModelsWithTakenNames();
    void deletesModelsNoLongerImported();
    void removesEmptyDirectory();
    void reportsMissingModel();

private:
    QString projectFile() const;
    QString modelsDirectory() const;

    QTemporaryDir _directory;
};

void TestProjectModels::init()
{
    QVERIFY(_directory.isValid());
    QDir(_directory.path()).removeRecursively();
    QDir().mkpath(_directory.path());
}

QString TestProjectModels::projectFile() const
{
    return _directory.filePath(QStringLiteral("plant.ouasim"));
}

QString TestProjectModels::modelsDirectory() const
{
    return _directory.filePath(QStringLiteral("plant.models"));
}

void TestProjectModels::directoryIsNamedAfterProject()
{
    QCOMPARE(ProjectModels::directory(projectFile()), modelsDirectory());
}

void TestProjectModels::copiesModelsNextToProject()
{
    const QString source = _directory.filePath(QStringLiteral("downloads/boiler.NodeSet2.xml"));
    QVERIFY(writeFile(source, "<UANodeSet/>"));

    QHash<QString, QString> moved;
    QString error;
    QVERIFY2(ProjectModels::store({ source }, projectFile(), moved, &error), qPrintable(error));
    const QString copy = modelsDirectory() + QStringLiteral("/boiler.NodeSet2.xml");
    QCOMPARE(moved, (QHash<QString, QString> { { source, copy } }));
    QCOMPARE(readFile(copy), QByteArray("<UANodeSet/>"));
    QVERIFY(QFile::exists(source));
}

void TestProjectModels::keepsModelsAlreadyThere()
{
    const QString stored = modelsDirectory() + QStringLiteral("/boiler.NodeSet2.xml");
    QVERIFY(writeFile(stored, "<UANodeSet/>"));

    QHash<QString, QString> moved;
    QVERIFY(ProjectModels::store({ stored }, projectFile(), moved));
    QVERIFY(moved.isEmpty());
    QCOMPARE(readFile(stored), QByteArray("<UANodeSet/>"));
}

void TestProjectModels::renamesModelsWithTakenNames()
{
    const QString stored = modelsDirectory() + QStringLiteral("/boiler.NodeSet2.xml");
    const QString other = _directory.filePath(QStringLiteral("other/boiler.NodeSet2.xml"));
    QVERIFY(writeFile(stored, "first"));
    QVERIFY(writeFile(other, "second"));

    QHash<QString, QString> moved;
    QVERIFY(ProjectModels::store({ stored, other }, projectFile(), moved));
    const QString copy = modelsDirectory() + QStringLiteral("/boiler_2.NodeSet2.xml");
    QCOMPARE(moved.value(other), copy);
    QCOMPARE(readFile(stored), QByteArray("first"));
    QCOMPARE(readFile(copy), QByteArray("second"));
}

void TestProjectModels::deletesModelsNoLongerImported()
{
    const QString kept = modelsDirectory() + QStringLiteral("/boiler.NodeSet2.xml");
    const QString dropped = modelsDirectory() + QStringLiteral("/tank.NodeSet2.xml");
    const QString notes = modelsDirectory() + QStringLiteral("/notes.txt");
    QVERIFY(writeFile(kept, "kept"));
    QVERIFY(writeFile(dropped, "dropped"));
    QVERIFY(writeFile(notes, "notes"));

    QHash<QString, QString> moved;
    QVERIFY(ProjectModels::store({ kept }, projectFile(), moved));
    QVERIFY(QFile::exists(kept));
    QVERIFY(!QFile::exists(dropped));
    QVERIFY(QFile::exists(notes));
}

void TestProjectModels::removesEmptyDirectory()
{
    QVERIFY(writeFile(modelsDirectory() + QStringLiteral("/tank.NodeSet2.xml"), "dropped"));

    QHash<QString, QString> moved;
    QVERIFY(ProjectModels::store({}, projectFile(), moved));
    QVERIFY(!QFileInfo::exists(modelsDirectory()));
}

void TestProjectModels::reportsMissingModel()
{
    QHash<QString, QString> moved;
    QString error;
    QVERIFY(!ProjectModels::store({ _directory.filePath(QStringLiteral("missing.NodeSet2.xml")) }, projectFile(), moved,
                                  &error));
    QVERIFY(error.contains(QStringLiteral("missing.NodeSet2.xml")));
    QVERIFY(moved.isEmpty());
}

QTEST_GUILESS_MAIN(TestProjectModels)

#include "test_projectmodels.moc"
