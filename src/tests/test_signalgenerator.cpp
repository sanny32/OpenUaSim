// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QRandomGenerator>
#include <QTest>

#include "simulation/signalgenerator.h"

namespace {

SimulationSettings settings(SimulationFunction function, double amplitude = 2.0, double offset = 10.0)
{
    SimulationSettings result;
    result.function = function;
    result.periodMs = 1000;
    result.amplitude = amplitude;
    result.offset = offset;
    return result;
}

}

Q_DECLARE_METATYPE(SimulationFunction)

class TestSignalGenerator : public QObject
{
    Q_OBJECT

private slots:
    void waveform_data();
    void waveform();
    void noneKeepsOffset();
    void periodRepeats();
    void negativeTimeWrapsIntoPeriod();
    void zeroPeriodDoesNotDivideByZero();
    void randomStaysWithinAmplitude();
    void randomIsReproducibleWithSeed();
    void keysRoundTrip();
    void unknownKeyFallsBackToNone();
    void displayNamesAreDistinct();
};

void TestSignalGenerator::waveform_data()
{
    QTest::addColumn<SimulationFunction>("function");
    QTest::addColumn<qint64>("elapsedMs");
    QTest::addColumn<double>("expected");

    QTest::newRow("sine start") << SimulationFunction::Sine << qint64(0) << 10.0;
    QTest::newRow("sine quarter") << SimulationFunction::Sine << qint64(250) << 12.0;
    QTest::newRow("sine three quarters") << SimulationFunction::Sine << qint64(750) << 8.0;
    QTest::newRow("square first half") << SimulationFunction::Square << qint64(100) << 12.0;
    QTest::newRow("square second half") << SimulationFunction::Square << qint64(600) << 8.0;
    QTest::newRow("triangle start") << SimulationFunction::Triangle << qint64(0) << 8.0;
    QTest::newRow("triangle peak") << SimulationFunction::Triangle << qint64(500) << 12.0;
    QTest::newRow("triangle falling") << SimulationFunction::Triangle << qint64(750) << 10.0;
    QTest::newRow("ramp start") << SimulationFunction::Ramp << qint64(0) << 8.0;
    QTest::newRow("ramp middle") << SimulationFunction::Ramp << qint64(500) << 10.0;
    QTest::newRow("step first level") << SimulationFunction::Step << qint64(0) << 8.0;
    QTest::newRow("step middle level") << SimulationFunction::Step << qint64(500) << 10.0;
    QTest::newRow("step last level") << SimulationFunction::Step << qint64(999) << 12.0;
}

void TestSignalGenerator::waveform()
{
    QFETCH(SimulationFunction, function);
    QFETCH(qint64, elapsedMs);
    QFETCH(double, expected);

    QRandomGenerator random(1);
    const double actual = SignalGenerator::value(settings(function), elapsedMs, random);
    QVERIFY2(qAbs(actual - expected) < 1e-9, qPrintable(QString::number(actual)));
}

void TestSignalGenerator::noneKeepsOffset()
{
    QRandomGenerator random(1);
    QCOMPARE(SignalGenerator::value(settings(SimulationFunction::None), 123, random), 10.0);
}

void TestSignalGenerator::periodRepeats()
{
    QRandomGenerator random(1);
    const SimulationSettings sine = settings(SimulationFunction::Sine);
    QVERIFY(qAbs(SignalGenerator::value(sine, 1250, random) - SignalGenerator::value(sine, 250, random)) < 1e-9);
}

void TestSignalGenerator::negativeTimeWrapsIntoPeriod()
{
    QRandomGenerator random(1);
    const SimulationSettings ramp = settings(SimulationFunction::Ramp);
    QCOMPARE(SignalGenerator::value(ramp, -500, random), SignalGenerator::value(ramp, 500, random));
}

void TestSignalGenerator::zeroPeriodDoesNotDivideByZero()
{
    QRandomGenerator random(1);
    SimulationSettings ramp = settings(SimulationFunction::Ramp);
    ramp.periodMs = 0;
    const double value = SignalGenerator::value(ramp, 12345, random);
    QVERIFY(qIsFinite(value));
    QCOMPARE(value, 8.0);
}

void TestSignalGenerator::randomStaysWithinAmplitude()
{
    QRandomGenerator random(42);
    const SimulationSettings noise = settings(SimulationFunction::Random);
    for (int i = 0; i < 1000; ++i) {
        const double value = SignalGenerator::value(noise, i, random);
        QVERIFY(value >= 8.0 && value <= 12.0);
    }
}

void TestSignalGenerator::randomIsReproducibleWithSeed()
{
    QRandomGenerator first(7);
    QRandomGenerator second(7);
    const SimulationSettings noise = settings(SimulationFunction::Random);
    for (int i = 0; i < 10; ++i)
        QCOMPARE(SignalGenerator::value(noise, i, first), SignalGenerator::value(noise, i, second));
}

void TestSignalGenerator::keysRoundTrip()
{
    for (SimulationFunction function : SignalGenerator::functions()) {
        bool ok = false;
        QCOMPARE(SignalGenerator::fromKey(SignalGenerator::key(function), &ok), function);
        QVERIFY(ok);
    }
}

void TestSignalGenerator::unknownKeyFallsBackToNone()
{
    bool ok = true;
    QCOMPARE(SignalGenerator::fromKey(QStringLiteral("sawtooth"), &ok), SimulationFunction::None);
    QVERIFY(!ok);
    QCOMPARE(SignalGenerator::fromKey(QString()), SimulationFunction::None);
}

void TestSignalGenerator::displayNamesAreDistinct()
{
    QStringList names;
    for (SimulationFunction function : SignalGenerator::functions())
        names << SignalGenerator::displayName(function);
    QCOMPARE(names.first(), QStringLiteral("None"));
    names.removeDuplicates();
    QCOMPARE(names.size(), SignalGenerator::functions().size());
}

QTEST_GUILESS_MAIN(TestSignalGenerator)

#include "test_signalgenerator.moc"
