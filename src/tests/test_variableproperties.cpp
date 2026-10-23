// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include <QUaProperty>
#include <QUaServer>

#include "server/variableproperties.h"

namespace {

const QUaNodeId kAnalogItemType(0, 2368u);
const QUaNodeId kTwoStateDiscreteType(0, 2373u);

QUaBaseDataVariable *addInstance(QUaServer &server, const QString &name, const QUaNodeId &typeId)
{
    return qobject_cast<QUaBaseDataVariable *>(server.objectsFolder()->addInstance(
        QUaQualifiedName(1, name), typeId, QUaNodeId(1, name)));
}

QStringList names(const QList<PropertyInfo> &properties)
{
    QStringList result;
    for (const PropertyInfo &property : properties)
        result.append(property.browseName);
    return result;
}

PropertyInfo findProperty(const QList<PropertyInfo> &properties, const QString &name)
{
    for (const PropertyInfo &info : properties) {
        if (info.browseName == name)
            return info;
    }
    return {};
}

}

class TestVariableProperties : public QObject
{
    Q_OBJECT

private slots:
    void textRoundTrips_data();
    void textRoundTrips();
    void rejectsInvalidText_data();
    void rejectsInvalidText();
    void describesAnalogItemProperties();
    void appliesValuesAndAddsOptionalProperties();
    void reportsUnknownPropertiesAndInvalidValues();
    void describesTwoStateDiscreteProperties();
    void baseVariableHasNoProperties();
};

void TestVariableProperties::textRoundTrips_data()
{
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expected");

    QTest::newRow("range") << "Range" << " -5.5 ;1e3 " << "-5.5; 1000";
    QTest::newRow("units") << "EUInformation" << "°C" << "°C";
    QTest::newRow("localized text") << "LocalizedText" << "Running" << "Running";
    QTest::newRow("localized texts") << "LocalizedText[]" << "Off;; On ; Fault" << "Off; On; Fault";
    QTest::newRow("enum values") << "EnumValueType[]" << "0: Off; 1 :On" << "0: Off; 1: On";
    QTest::newRow("double") << "Double" << "0.25" << "0.25";
    QTest::newRow("string") << "String" << "Level of tank 1" << "Level of tank 1";
    QTest::newRow("doubles") << "Double[]" << "1; 2.5" << "1; 2.5";
}

void TestVariableProperties::textRoundTrips()
{
    QFETCH(QString, dataType);
    QFETCH(QString, text);
    QFETCH(QString, expected);

    QVERIFY(VariableProperties::isSupported(dataType));
    const QVariant value = VariableProperties::fromText(text, dataType);
    QVERIFY(value.isValid());
    QCOMPARE(VariableProperties::toText(value, dataType), expected);
}

void TestVariableProperties::rejectsInvalidText_data()
{
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<QString>("text");

    QTest::newRow("empty") << "Range" << "  ";
    QTest::newRow("one limit") << "Range" << "5";
    QTest::newRow("no number") << "Range" << "low; high";
    QTest::newRow("enum value without number") << "EnumValueType[]" << "Off";
    QTest::newRow("double") << "Double" << "fast";
    QTest::newRow("scalar enum value") << "EnumValueType" << "0: Off";
    QTest::newRow("unsupported type") << "NodeId" << "i=85";
}

void TestVariableProperties::rejectsInvalidText()
{
    QFETCH(QString, dataType);
    QFETCH(QString, text);

    QVERIFY(!VariableProperties::fromText(text, dataType).isValid());
}

void TestVariableProperties::describesAnalogItemProperties()
{
    QUaServer server;
    QUaBaseDataVariable *level = addInstance(server, QStringLiteral("Level"), kAnalogItemType);
    QVERIFY(level);

    const QList<PropertyInfo> properties = VariableProperties::describe(server, *level);
    QCOMPARE(names(properties), QStringList({ QStringLiteral("EURange"), QStringLiteral("EngineeringUnits"),
                                              QStringLiteral("InstrumentRange"), QStringLiteral("Definition"),
                                              QStringLiteral("ValuePrecision") }));
    const PropertyInfo range = findProperty(properties, QStringLiteral("EURange"));
    QCOMPARE(range.dataType, QStringLiteral("Range"));
    QCOMPARE(range.value, QStringLiteral("0; 0"));
    QVERIFY2(findProperty(properties, QStringLiteral("EngineeringUnits")).value.isEmpty(),
             "optional properties are listed before they are added");
    QVERIFY(range.editable);
    QVERIFY(!range.edited);
    QCOMPARE(findProperty(properties, QStringLiteral("EngineeringUnits")).dataType, QStringLiteral("EUInformation"));
}

void TestVariableProperties::appliesValuesAndAddsOptionalProperties()
{
    QUaServer server;
    QUaBaseDataVariable *level = addInstance(server, QStringLiteral("Level"), kAnalogItemType);
    QVERIFY(level);

    const QStringList problems = VariableProperties::apply(
        server, *level,
        { { QStringLiteral("EURange"), QStringLiteral("0; 100") },
          { QStringLiteral("EngineeringUnits"), QStringLiteral("m³") } });
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QLatin1Char('\n'))));

    auto *units = level->browseChild<QUaProperty>(QUaQualifiedName(0, QStringLiteral("EngineeringUnits")));
    QVERIFY(units);
    QCOMPARE(units->nodeId(), QUaNodeId(1, QStringLiteral("Level.EngineeringUnits")));
    QCOMPARE(units->dataTypeNodeId(), QStringLiteral("ns=0;i=887"));
    QVERIFY2(!level->browseChild(QUaQualifiedName(0, QStringLiteral("InstrumentRange"))),
             "optional properties without a value are not added");

    const QList<PropertyInfo> properties = VariableProperties::describe(server, *level);
    QCOMPARE(findProperty(properties, QStringLiteral("EURange")).value, QStringLiteral("0; 100"));
    QCOMPARE(findProperty(properties, QStringLiteral("EngineeringUnits")).value, QStringLiteral("m³"));
    QCOMPARE(properties.size(), 5);

    QVERIFY(VariableProperties::apply(server, *level, { { QStringLiteral("EngineeringUnits"), QStringLiteral("l") } })
                .isEmpty());
    QCOMPARE(findProperty(VariableProperties::describe(server, *level), QStringLiteral("EngineeringUnits")).value,
             QStringLiteral("l"));
}

void TestVariableProperties::reportsUnknownPropertiesAndInvalidValues()
{
    QUaServer server;
    QUaBaseDataVariable *level = addInstance(server, QStringLiteral("Level"), kAnalogItemType);
    QVERIFY(level);

    const QStringList problems = VariableProperties::apply(
        server, *level,
        { { QStringLiteral("TrueState"), QStringLiteral("On") }, { QStringLiteral("EURange"), QStringLiteral("5") } });
    QCOMPARE(problems.size(), 2);
    QVERIFY(problems.contains(QStringLiteral("the type declares no property 'TrueState'")));
    QVERIFY(problems.contains(QStringLiteral("'5' is not a Range value for 'EURange'")));
    QVERIFY(!level->browseChild(QUaQualifiedName(0, QStringLiteral("TrueState"))));
}

void TestVariableProperties::describesTwoStateDiscreteProperties()
{
    QUaServer server;
    QUaBaseDataVariable *running = addInstance(server, QStringLiteral("Running"), kTwoStateDiscreteType);
    QVERIFY(running);

    QVERIFY(VariableProperties::apply(server, *running, { { QStringLiteral("TrueState"), QStringLiteral("Running") } })
                .isEmpty());
    const QList<PropertyInfo> properties = VariableProperties::describe(server, *running);
    QCOMPARE(names(properties).mid(0, 2), QStringList({ QStringLiteral("TrueState"), QStringLiteral("FalseState") }));
    QCOMPARE(findProperty(properties, QStringLiteral("TrueState")).dataType, QStringLiteral("LocalizedText"));
    QCOMPARE(findProperty(properties, QStringLiteral("TrueState")).value, QStringLiteral("Running"));
    QVERIFY(!names(properties).contains(QStringLiteral("EURange")));
}

void TestVariableProperties::baseVariableHasNoProperties()
{
    QUaServer server;
    QUaBaseDataVariable *plain = server.objectsFolder()->addBaseDataVariable(QUaQualifiedName(1, QStringLiteral("Plain")));
    QVERIFY(VariableProperties::describe(server, *plain).isEmpty());

    QUaProperty *note = plain->addProperty(QUaQualifiedName(1, QStringLiteral("Note")));
    note->setValue(QStringLiteral("spare"));
    const QList<PropertyInfo> properties = VariableProperties::describe(server, *plain);
    QCOMPARE(properties.size(), 1);
    QCOMPARE(properties.first().dataType, QStringLiteral("String"));
    QCOMPARE(properties.first().value, QStringLiteral("spare"));
    QVERIFY(VariableProperties::apply(server, *plain, { { QStringLiteral("Note"), QStringLiteral("used") } }).isEmpty());
    QCOMPARE(note->value().toString(), QStringLiteral("used"));
}

QTEST_GUILESS_MAIN(TestVariableProperties)

#include "test_variableproperties.moc"
