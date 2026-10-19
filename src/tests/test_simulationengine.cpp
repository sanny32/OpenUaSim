// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QSignalSpy>
#include <QTest>

#include <QUaServer>

#include "project/defaultproject.h"
#include "server/addressspace.h"
#include "simulation/simulationengine.h"
#include "simulation/valuesink.h"

namespace {

class FakeSink : public ValueSink
{
public:
    bool setValue(const QString &nodeId, const QVariant &value) override
    {
        if (missing.contains(nodeId))
            return false;
        values.insert(nodeId, value);
        ++writes;
        return true;
    }

    QHash<QString, QVariant> values;
    QStringList missing;
    int writes = 0;
};

SimulatedVariable simulated(const QString &nodeId, SimulationFunction function, int intervalMs,
                            const QString &dataType = QStringLiteral("Double"))
{
    SimulatedVariable variable;
    variable.nodeId = nodeId;
    variable.dataType = dataType;
    variable.settings.function = function;
    variable.settings.intervalMs = intervalMs;
    variable.settings.periodMs = 1000;
    variable.settings.amplitude = 10.0;
    variable.settings.offset = 100.0;
    return variable;
}

}

class TestSimulationEngine : public QObject
{
    Q_OBJECT

private slots:
    void collectsSimulatedVariablesOfProject();
    void ignoresFunctionNone();
    void oneTimerPerInterval();
    void clampsFastIntervals();
    void tickWritesOnlyItsInterval();
    void tickFollowsElapsedTime();
    void shiftsArrayElements();
    void convertsToDataType();
    void skipsMissingVariables();
    void updateAndRemoveRebuildTimers();
    void timersDriveValuesWhileRunning();
    void stopFreezesValues();
    void drivesAddressSpace();
};

void TestSimulationEngine::collectsSimulatedVariablesOfProject()
{
    const QList<SimulatedVariable> variables = SimulationEngine::variablesOf(DefaultProject::create().nodes);
    QCOMPARE(variables.size(), 8);
    QCOMPARE(variables.first().nodeId, QStringLiteral("ns=1;s=Motor_01.Speed"));
    QCOMPARE(variables.first().settings.function, SimulationFunction::Sine);
}

void TestSimulationEngine::ignoresFunctionNone()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setVariables({ simulated(QStringLiteral("a"), SimulationFunction::None, 100) });
    QVERIFY(engine.variableIds().isEmpty());
    QVERIFY(engine.intervals().isEmpty());
}

void TestSimulationEngine::oneTimerPerInterval()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setVariables({ simulated(QStringLiteral("a"), SimulationFunction::Sine, 100),
                          simulated(QStringLiteral("b"), SimulationFunction::Ramp, 100),
                          simulated(QStringLiteral("c"), SimulationFunction::Square, 500) });
    QCOMPARE(engine.intervals(), QList<int>({ 100, 500 }));
}

void TestSimulationEngine::clampsFastIntervals()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setVariables({ simulated(QStringLiteral("a"), SimulationFunction::Sine, 0) });
    QCOMPARE(engine.intervals(), QList<int>({ SimulationEngine::kMinimumIntervalMs }));
}

void TestSimulationEngine::tickWritesOnlyItsInterval()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setElapsedSource([] { return qint64(250); });
    engine.setVariables({ simulated(QStringLiteral("fast"), SimulationFunction::Sine, 100),
                          simulated(QStringLiteral("slow"), SimulationFunction::Sine, 500) });
    QSignalSpy updated(&engine, &SimulationEngine::valuesUpdated);

    engine.tick(100);
    QCOMPARE(sink.values.keys(), QStringList { QStringLiteral("fast") });
    QCOMPARE(sink.values.value(QStringLiteral("fast")).toDouble(), 110.0);
    QCOMPARE(updated.count(), 1);
    using Values = QHash<QString, QVariant>;
    QCOMPARE(updated.first().first().value<Values>().size(), 1);
}

void TestSimulationEngine::tickFollowsElapsedTime()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    qint64 now = 0;
    engine.setElapsedSource([&now] { return now; });
    engine.setVariables({ simulated(QStringLiteral("ramp"), SimulationFunction::Ramp, 100) });

    engine.tick(100);
    QCOMPARE(sink.values.value(QStringLiteral("ramp")).toDouble(), 90.0);
    now = 500;
    engine.tick(100);
    QCOMPARE(sink.values.value(QStringLiteral("ramp")).toDouble(), 100.0);
}

void TestSimulationEngine::shiftsArrayElements()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setElapsedSource([] { return qint64(0); });
    SimulatedVariable ramp = simulated(QStringLiteral("ramp"), SimulationFunction::Ramp, 100);
    ramp.arrayLength = 4;
    engine.setVariables({ ramp });

    engine.tick(100);
    QCOMPARE(sink.values.value(QStringLiteral("ramp")).toList(), QVariantList({ 90.0, 95.0, 100.0, 105.0 }));
}

void TestSimulationEngine::convertsToDataType()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setElapsedSource([] { return qint64(250); });
    engine.setVariables({ simulated(QStringLiteral("i"), SimulationFunction::Sine, 100, QStringLiteral("Int16")) });
    engine.tick(100);
    const QVariant value = sink.values.value(QStringLiteral("i"));
    QCOMPARE(value.typeId(), int(QMetaType::Short));
    QCOMPARE(value.toInt(), 110);
}

void TestSimulationEngine::skipsMissingVariables()
{
    FakeSink sink;
    sink.missing = { QStringLiteral("gone") };
    SimulationEngine engine(&sink);
    engine.setVariables({ simulated(QStringLiteral("gone"), SimulationFunction::Sine, 100) });
    QSignalSpy updated(&engine, &SimulationEngine::valuesUpdated);
    engine.tick(100);
    QCOMPARE(updated.count(), 0);
}

void TestSimulationEngine::updateAndRemoveRebuildTimers()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setVariables({ simulated(QStringLiteral("a"), SimulationFunction::Sine, 100) });

    engine.updateVariable(simulated(QStringLiteral("a"), SimulationFunction::Sine, 250));
    QCOMPARE(engine.intervals(), QList<int>({ 250 }));

    engine.updateVariable(simulated(QStringLiteral("b"), SimulationFunction::Random, 100));
    QCOMPARE(engine.intervals(), QList<int>({ 100, 250 }));

    engine.updateVariable(simulated(QStringLiteral("a"), SimulationFunction::None, 250));
    QCOMPARE(engine.variableIds(), QStringList { QStringLiteral("b") });
    QCOMPARE(engine.intervals(), QList<int>({ 100 }));

    engine.removeVariable(QStringLiteral("b"));
    QVERIFY(engine.intervals().isEmpty());
}

void TestSimulationEngine::timersDriveValuesWhileRunning()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setVariables({ simulated(QStringLiteral("a"), SimulationFunction::Random, 20) });
    QVERIFY(!engine.isRunning());
    QTest::qWait(60);
    QCOMPARE(sink.writes, 0);

    engine.start();
    QVERIFY(engine.isRunning());
    QTRY_VERIFY(sink.writes >= 3);

    engine.updateVariable(simulated(QStringLiteral("b"), SimulationFunction::Random, 30));
    QTRY_VERIFY(sink.values.contains(QStringLiteral("b")));
}

void TestSimulationEngine::stopFreezesValues()
{
    FakeSink sink;
    SimulationEngine engine(&sink);
    engine.setVariables({ simulated(QStringLiteral("a"), SimulationFunction::Random, 20) });
    engine.start();
    QTRY_VERIFY(sink.writes > 0);
    engine.stop();
    const int writes = sink.writes;
    QTest::qWait(80);
    QCOMPARE(sink.writes, writes);
}

void TestSimulationEngine::drivesAddressSpace()
{
    QUaServer server;
    AddressSpace space(&server);
    const ProjectConfig project = DefaultProject::create();
    QVERIFY(space.build(project.nodes).isEmpty());

    SimulationEngine engine(&space);
    engine.setElapsedSource([] { return qint64(2500); });
    engine.setVariables(SimulationEngine::variablesOf(project.nodes));
    engine.tick(100);
    QCOMPARE(space.value(QStringLiteral("ns=1;s=Motor_01.Speed")).toDouble(), 1500.0);
}

QTEST_GUILESS_MAIN(TestSimulationEngine)

#include "test_simulationengine.moc"
