// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include <QUaBaseDataVariable>
#include <QUaServer>

#include "server/modelvariables.h"
#include "server/simserver.h"

namespace {

const QString kTankUri = QStringLiteral("urn:openuasim:test:tank");

ModelVariable stored(const QString &namespaceUri, const QString &identifier)
{
    ModelVariable variable;
    variable.namespaceUri = namespaceUri;
    variable.variable.kind = NodeDefinition::Kind::Variable;
    variable.variable.nodeId = identifier;
    return variable;
}

}

class TestModelVariables : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void findsModelVariablesOnly();
    void describesVariables();
    void tellsWhichVariablesAreEditable();
    void findsStoredSettings();
    void resolvesStoredSettings();
    void reportsUnresolvableSettings();

private:
    QString serverId(const QString &identifier) const;
    QUaBaseDataVariable &variable(const QString &identifier) const;

    SimServer _server;
    QUaServer *_ua = nullptr;
};

void TestModelVariables::initTestCase()
{
    _ua = _server.uaServer();
    QVERIFY(_ua->loadNodeSet(QStringLiteral(OUASIM_TEST_DATA_DIR "/tank.NodeSet2.xml")).isOk());
    QVERIFY(_ua->namespaces().contains(kTankUri));
}

QString TestModelVariables::serverId(const QString &identifier) const
{
    return QStringLiteral("ns=%1;%2").arg(_ua->namespaces().indexOf(kTankUri)).arg(identifier);
}

QUaBaseDataVariable &TestModelVariables::variable(const QString &identifier) const
{
    QUaBaseDataVariable *found = ModelVariables::find(*_ua, serverId(identifier));
    if (!found)
        qFatal("Tank model has no variable %s", qPrintable(identifier));
    return *found;
}

void TestModelVariables::findsModelVariablesOnly()
{
    QVERIFY(ModelVariables::find(*_ua, serverId(QStringLiteral("i=1001"))));
    QVERIFY(!ModelVariables::find(*_ua, serverId(QStringLiteral("i=9999"))));
    QVERIFY(!ModelVariables::find(*_ua, serverId(QStringLiteral("i=1000"))));
    QVERIFY(!ModelVariables::find(*_ua, QStringLiteral("i=2256")));
    QVERIFY(!ModelVariables::find(*_ua, QStringLiteral("not a node id")));
}

void TestModelVariables::describesVariables()
{
    const ModelVariable level = ModelVariables::describe(*_ua, variable(QStringLiteral("i=1001")));
    QCOMPARE(level.namespaceUri, kTankUri);
    QCOMPARE(level.variable.nodeId, QStringLiteral("i=1001"));
    QCOMPARE(level.variable.browseName, QStringLiteral("Level"));
    QCOMPARE(level.variable.dataType, QStringLiteral("Double"));
    QCOMPARE(level.variable.access, QStringLiteral("Read"));
    QCOMPARE(level.variable.arrayLength, 0);
    QVERIFY(!level.variable.initialValue.isValid());

    QUaBaseDataVariable &mode = variable(QStringLiteral("i=1003"));
    QCOMPARE(ModelVariables::describe(*_ua, mode).variable.access, QStringLiteral("Read/Write"));
    QCOMPARE(ModelVariables::valueDataType(*_ua, mode), QStringLiteral("Int32"));
    QCOMPARE(ModelVariables::dataTypeName(*_ua, mode), QStringLiteral("TankMode"));
    QVERIFY(ModelVariables::dataTypeName(*_ua, variable(QStringLiteral("i=1001"))).isEmpty());
}

void TestModelVariables::tellsWhichVariablesAreEditable()
{
    QVERIFY(ModelVariables::isEditable(*_ua, variable(QStringLiteral("i=1001"))));
    QVERIFY(ModelVariables::isEditable(*_ua, variable(QStringLiteral("i=1003"))));
    QVERIFY(!ModelVariables::isEditable(*_ua, variable(QStringLiteral("i=1002"))));
}

void TestModelVariables::findsStoredSettings()
{
    const QList<ModelVariable> variables = { stored(kTankUri, QStringLiteral("i=1001")),
                                             stored(kTankUri, QStringLiteral("i=1003")) };
    QCOMPARE(ModelVariables::indexOf(variables, kTankUri, QStringLiteral("i=1003")), 1);
    QCOMPARE(ModelVariables::indexOf(variables, kTankUri, QStringLiteral("i=1002")), -1);
    QCOMPARE(ModelVariables::indexOf(variables, QStringLiteral("urn:other"), QStringLiteral("i=1001")), -1);
    QCOMPARE(ModelVariables::indexOf({}, kTankUri, QStringLiteral("i=1001")), -1);
}

void TestModelVariables::resolvesStoredSettings()
{
    ModelVariable mode = stored(kTankUri, QStringLiteral("i=1003"));
    mode.variable.browseName = QStringLiteral("Stale name");
    mode.variable.dataType = QStringLiteral("String");
    mode.variable.simulation.function = SimulationFunction::Counter;

    QStringList problems;
    const QList<NodeDefinition> resolved = ModelVariables::resolve({ mode }, *_ua, &problems);
    QVERIFY(problems.isEmpty());
    QCOMPARE(resolved.size(), 1);
    QCOMPARE(resolved.first().nodeId, serverId(QStringLiteral("i=1003")));
    QCOMPARE(resolved.first().browseName, QStringLiteral("Mode"));
    QCOMPARE(resolved.first().dataType, QStringLiteral("Int32"));
    QCOMPARE(resolved.first().simulation.function, SimulationFunction::Counter);
}

void TestModelVariables::reportsUnresolvableSettings()
{
    const QList<ModelVariable> variables = { stored(QStringLiteral("urn:not:loaded"), QStringLiteral("i=1001")),
                                             stored(kTankUri, QStringLiteral("i=9999")),
                                             stored(kTankUri, QStringLiteral("i=1002")),
                                             stored(kTankUri, QStringLiteral("i=1001")) };
    QStringList problems;
    const QList<NodeDefinition> resolved = ModelVariables::resolve(variables, *_ua, &problems);
    QCOMPARE(resolved.size(), 1);
    QCOMPARE(resolved.first().browseName, QStringLiteral("Level"));
    QCOMPARE(problems.size(), 3);
    QVERIFY(problems.at(0).startsWith(QStringLiteral("urn:not:loaded;i=1001: ")));
    QVERIFY(problems.at(0).contains(QStringLiteral("no imported model")));
    QVERIFY(problems.at(1).contains(QStringLiteral("no such variable")));
    QVERIFY(problems.at(2).contains(QStringLiteral("cannot drive")));

    QCOMPARE(ModelVariables::resolve(variables, *_ua).size(), 1);
}

QTEST_GUILESS_MAIN(TestModelVariables)
#include "test_modelvariables.moc"
