// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QAbstractItemModelTester>
#include <QLocale>
#include <QSignalSpy>
#include <QTest>

#include "certificatesmodel.h"
#include "endpointsmodel.h"
#include "itemroles.h"
#include "logmodel.h"
#include "namespacesmodel.h"
#include "referencesmodel.h"
#include "sessionsmodel.h"
#include "subscriptionsmodel.h"
#include "subtypesmodel.h"
#include "variablelisting.h"
#include "variablesmodel.h"

namespace {

VariableInfo variable(const QString &nodeId, const QVariant &value,
                      VariableInfo::Quality quality = VariableInfo::Quality::Good)
{
    VariableInfo info;
    info.nodeId = nodeId;
    info.displayName = nodeId.section(QLatin1Char('.'), -1);
    info.icon = QStringLiteral("signal");
    info.value = value;
    info.dataType = QStringLiteral("Double");
    info.quality = quality;
    return info;
}

QString header(const QAbstractItemModel &model, int section)
{
    return model.headerData(section, Qt::Horizontal).toString();
}

StatusKind status(const QModelIndex &index)
{
    return ItemRoles::statusFrom(index.data(ItemRoles::Status));
}

TreeNode treeNode(const QString &text, TreeNode::Kind kind, const QList<TreeNode> &children = {})
{
    TreeNode node;
    node.text = text;
    node.id = text;
    node.kind = kind;
    node.children = children;
    return node;
}

///
/// \brief Builds Line with its own Mode variable, a Motor object with a nested Gearbox and an empty Tank.
///
TreeNode lineTree()
{
    const TreeNode gearbox = treeNode(QStringLiteral("Gearbox"), TreeNode::Kind::Object,
                                      { treeNode(QStringLiteral("Gearbox.Oil"), TreeNode::Kind::Variable) });
    const TreeNode motor = treeNode(QStringLiteral("Motor"), TreeNode::Kind::Object,
                                    { treeNode(QStringLiteral("Motor.Speed"), TreeNode::Kind::Variable),
                                      treeNode(QStringLiteral("Motor.Start"), TreeNode::Kind::Method), gearbox });
    return treeNode(QStringLiteral("Line"), TreeNode::Kind::Folder,
                    { treeNode(QStringLiteral("Line.Mode"), TreeNode::Kind::Variable), motor,
                      treeNode(QStringLiteral("Tank"), TreeNode::Kind::Object) });
}

QHash<QString, QList<VariableInfo>> lineVariables()
{
    return { { QStringLiteral("Line"), { variable(QStringLiteral("Line.Mode"), 1) } },
             { QStringLiteral("Motor"),
               { variable(QStringLiteral("Motor.Speed"), 2.0), variable(QStringLiteral("Motor.Load"), 3.0) } },
             { QStringLiteral("Gearbox"), { variable(QStringLiteral("Gearbox.Oil"), 4.0) } } };
}

QStringList nodeIds(const QList<VariableInfo> &variables)
{
    QStringList ids;
    for (const VariableInfo &info : variables)
        ids.append(info.nodeId);
    return ids;
}

}

class TestTableModels : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void variablesHaveChildRows();
    void itemTableModelStoresItems();
    void itemTableModelReplacesItem();
    void itemTableModelAppendsWithLimit();
    void variablesModelUpdatesValues();
    void variablesModelShowsArrays();
    void variablesModelShowsModelDataTypeNames();
    void variablesModelShowsNodeTypes();
    void variablesModelShowsAlarmState();
    void variablesModelShowsLocations();
    void variableListingFindsNodes();
    void variableListingCollectsOwnVariables();
    void variableListingCollectsNestedVariables();
    void variableListingCollectsChildrenOfVariables();
    void subscriptionsModel();
    void tableModelsHaveNoChildren();
    void variablesModel();
    void variablesModelQuality_data();
    void variablesModelQuality();
    void variablesModelPassesTester();
    void subtypesModel();
    void referencesModel();
    void namespacesModel();
    void endpointsModel();
    void endpointUserAuthenticationText_data();
    void endpointUserAuthenticationText();
    void certificatesModel();
    void certificateStatus_data();
    void certificateStatus();
    void sessionsModel();
    void logModel();
    void logLevels_data();
    void logLevels();
    void logCategories();
};

void TestTableModels::variablesHaveChildRows()
{
    VariablesModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    const VariableInfo analog = variable(QStringLiteral("AI"), 0);
    const VariableInfo range = variable(QStringLiteral("AI.EURange"), 1);
    const VariableInfo orphan = variable(QStringLiteral("Other"), 2);
    model.setVariables({analog, range, orphan}, {},
                       {{range.nodeId, analog.nodeId}, {orphan.nodeId, QStringLiteral("missing")}});
    QCOMPARE(model.rowCount(), 2);
    const QModelIndex root = model.index(0, 0);
    QCOMPARE(model.rowCount(root), 1);
    QCOMPARE(model.rowCount(model.index(0, 1)), 0);
    const QModelIndex child = model.index(0, 0, root);
    QCOMPARE(child.parent(), root);
    QCOMPARE(child.data(ItemRoles::Id).toString(), range.nodeId);
    QCOMPARE(child.data().toString(), QStringLiteral("EURange"));
    QCOMPARE(model.indexForRow(model.rowOf(range.nodeId)), child);
    QVERIFY(!model.index(1, 0, root).isValid());
    QVERIFY(!model.indexForRow(-1).isValid());
    QSignalSpy changed(&model, &VariablesModel::dataChanged);
    model.updateValues({{range.nodeId, 7.5}, {QStringLiteral("missing"), 3}});
    QCOMPARE(changed.size(), 1);
    QCOMPARE(changed.first().at(0).toModelIndex().parent(), root);
    QCOMPARE(model.index(0, VariablesModel::ColValue, root).data().toString(), QStringLiteral("7.5"));
    model.setItems({range});
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);
    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

void TestTableModels::initTestCase()
{
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
}

void TestTableModels::itemTableModelStoresItems()
{
    VariablesModel model;
    QSignalSpy reset(&model, &VariablesModel::modelReset);

    model.setItems({ variable(QStringLiteral("a.Speed"), 1.0), variable(QStringLiteral("a.Load"), 2.0) });
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.items().size(), 2);
    QCOMPARE(model.item(1).displayName, QStringLiteral("Load"));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(reset.count(), 2);
    QVERIFY(!model.index(0, 0).data().isValid());
}

void TestTableModels::itemTableModelReplacesItem()
{
    VariablesModel model;
    model.setItems({ variable(QStringLiteral("a.Speed"), 1.0), variable(QStringLiteral("a.Load"), 2.0) });
    QSignalSpy changed(&model, &VariablesModel::dataChanged);

    model.setItem(1, variable(QStringLiteral("a.Flow"), 3.0));
    QCOMPARE(model.item(1).displayName, QStringLiteral("Flow"));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.first().at(0).toModelIndex(), model.index(1, 0));
    QCOMPARE(changed.first().at(1).toModelIndex(), model.index(1, VariablesModel::ColCount - 1));
}

void TestTableModels::itemTableModelAppendsWithLimit()
{
    LogModel model;
    QAbstractItemModelTester tester(&model);
    QSignalSpy inserted(&model, &LogModel::rowsInserted);
    QSignalSpy removed(&model, &LogModel::rowsRemoved);

    LogEntry entry;
    model.appendItems({});
    QCOMPARE(inserted.count(), 0);

    for (int i = 0; i < 5; ++i) {
        entry.message = QString::number(i);
        model.appendItems({ entry }, 3);
    }
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.item(0).message, QStringLiteral("2"));
    QCOMPARE(model.item(2).message, QStringLiteral("4"));
    QCOMPARE(inserted.count(), 5);
    QCOMPARE(removed.count(), 2);

    model.appendItems({ entry, entry });
    QCOMPARE(model.rowCount(), 5);
}

void TestTableModels::variablesModelUpdatesValues()
{
    VariablesModel model;
    model.setItems({ variable(QStringLiteral("a.Speed"), 1.0), variable(QStringLiteral("a.Load"), 2.0) });
    QSignalSpy changed(&model, &VariablesModel::dataChanged);

    model.updateValues({ { QStringLiteral("a.Load"), 7.5 }, { QStringLiteral("other"), 1.0 } });
    QCOMPARE(changed.count(), 1);
    QCOMPARE(model.item(0).value.toDouble(), 1.0);
    QCOMPARE(model.item(1).value.toDouble(), 7.5);
    QCOMPARE(model.index(1, VariablesModel::ColValue).data().toString(), QStringLiteral("7.5"));
}

void TestTableModels::variablesModelShowsArrays()
{
    VariableInfo levels = variable(QStringLiteral("a.Levels"), QVariantList { 1.5, 2.0 });
    levels.dataType = QStringLiteral("Double");
    levels.arrayLength = 2;
    VariablesModel model;
    model.setItems({ levels });
    QCOMPARE(model.index(0, VariablesModel::ColDataType).data().toString(), QStringLiteral("Double[2]"));
    QCOMPARE(model.index(0, VariablesModel::ColValue).data().toString(), QStringLiteral("[1.5, 2]"));
}

void TestTableModels::variablesModelShowsModelDataTypeNames()
{
    VariableInfo mode = variable(QStringLiteral("a.Mode"), 1);
    mode.dataType = QStringLiteral("Int32");
    mode.dataTypeName = QStringLiteral("TankMode");
    VariableInfo modes = mode;
    modes.arrayLength = 3;
    VariablesModel model;
    model.setItems({ mode, modes });
    QCOMPARE(model.index(0, VariablesModel::ColDataType).data().toString(), QStringLiteral("TankMode"));
    QCOMPARE(model.index(1, VariablesModel::ColDataType).data().toString(), QStringLiteral("TankMode[3]"));
}

void TestTableModels::variablesModelShowsNodeTypes()
{
    VariableInfo analog = variable(QStringLiteral("AI"), 0);
    analog.typeName = QStringLiteral("AnalogItemType");
    VariableInfo property = variable(QStringLiteral("AI.EURange"), 0);
    property.typeName = QStringLiteral("PropertyType");
    property.dataType = QStringLiteral("Range");
    property.property = true;
    const VariableInfo unknown = variable(QStringLiteral("Unknown"), 0);
    VariablesModel model;
    model.setVariables({analog, property, unknown}, {}, {{property.nodeId, analog.nodeId}});
    QCOMPARE(header(model, VariablesModel::ColNodeType), QStringLiteral("Node Type"));
    QCOMPARE(int(VariablesModel::ColNodeType), int(VariablesModel::ColDataType) + 1);
    const QModelIndex type = model.index(0, VariablesModel::ColNodeType);
    QCOMPARE(type.data().toString(), analog.typeName);
    QCOMPARE(type.data(Qt::ToolTipRole).toString(), analog.typeName);
    QCOMPARE(model.index(0, VariablesModel::ColDataType).data().toString(), QStringLiteral("Double"));
    const QModelIndex child = model.index(0, VariablesModel::ColNodeType, model.index(0, 0));
    QCOMPARE(child.data().toString(), property.typeName);
    QCOMPARE(child.data(Qt::ToolTipRole).toString(), property.typeName);
    QVERIFY(model.index(1, VariablesModel::ColNodeType).data().toString().isEmpty());
    analog.typeName = QStringLiteral("CustomAnalogType");
    model.setItem(0, analog);
    QCOMPARE(type.data().toString(), analog.typeName);
    QCOMPARE(type.data(Qt::ToolTipRole).toString(), analog.typeName);
}

void TestTableModels::variablesModelShowsAlarmState()
{
    VariableInfo plain = variable(QStringLiteral("a.Plain"), 1.0);
    VariableInfo normal = variable(QStringLiteral("a.Normal"), 1.0);
    normal.alarm.enabled = true;
    VariableInfo high = normal;
    high.nodeId = QStringLiteral("a.High");
    high.alarmState = QStringLiteral("High");
    VariablesModel model;
    model.setItems({ plain, normal, high });

    QCOMPARE(header(model, VariablesModel::ColAlarm), QStringLiteral("Alarm"));
    QVERIFY(model.index(0, VariablesModel::ColAlarm).data().toString().isEmpty());
    QCOMPARE(model.index(1, VariablesModel::ColAlarm).data().toString(), QStringLiteral("Normal"));
    QCOMPARE(model.index(1, VariablesModel::ColAlarm).data(ItemRoles::Status),
             ItemRoles::statusValue(StatusKind::Good));
    QCOMPARE(model.index(2, VariablesModel::ColAlarm).data().toString(), QStringLiteral("High"));
    QCOMPARE(model.index(2, VariablesModel::ColAlarm).data(ItemRoles::Status),
             ItemRoles::statusValue(StatusKind::Error));
}

void TestTableModels::variablesModelShowsLocations()
{
    VariablesModel model;
    model.setVariables({ variable(QStringLiteral("a.Speed"), 1.0), variable(QStringLiteral("b.Load"), 2.0) },
                       { { QStringLiteral("b.Load"), QStringLiteral("Line/Motor") } });
    QCOMPARE(model.index(0, VariablesModel::ColName).data().toString(), QStringLiteral("Speed"));
    QCOMPARE(model.index(1, VariablesModel::ColName).data().toString(), QStringLiteral("Line/Motor/Load"));
    QCOMPARE(model.item(1).displayName, QStringLiteral("Load"));

    model.setVariables({ variable(QStringLiteral("b.Load"), 2.0) });
    QCOMPARE(model.index(0, VariablesModel::ColName).data().toString(), QStringLiteral("Load"));
}

void TestTableModels::variableListingFindsNodes()
{
    const QList<TreeNode> tree = { treeNode(QStringLiteral("Root"), TreeNode::Kind::Folder), lineTree() };
    const TreeNode *gearbox = VariableListing::find(tree, QStringLiteral("Gearbox"));
    QVERIFY(gearbox);
    QCOMPARE(gearbox->kind, TreeNode::Kind::Object);
    QCOMPARE(VariableListing::find(tree, QStringLiteral("Root")), &tree.first());
    QVERIFY(!VariableListing::find(tree, QStringLiteral("Missing")));
    QVERIFY(!VariableListing::find({}, QStringLiteral("Line")));
}

void TestTableModels::variableListingCollectsOwnVariables()
{
    const TreeNode line = lineTree();
    const VariableListing::Listing listing = VariableListing::collect(line, lineVariables(), false);
    QCOMPARE(nodeIds(listing.variables), QStringList { QStringLiteral("Line.Mode") });
    QVERIFY(listing.locations.isEmpty());

    const TreeNode *tank = VariableListing::find({ line }, QStringLiteral("Tank"));
    QVERIFY(VariableListing::collect(*tank, lineVariables(), true).variables.isEmpty());
}

void TestTableModels::variableListingCollectsNestedVariables()
{
    const TreeNode line = lineTree();
    const VariableListing::Listing listing = VariableListing::collect(line, lineVariables(), true);
    QCOMPARE(nodeIds(listing.variables),
             (QStringList { QStringLiteral("Line.Mode"), QStringLiteral("Motor.Speed"), QStringLiteral("Motor.Load"),
                            QStringLiteral("Gearbox.Oil") }));
    QVERIFY(!listing.locations.contains(QStringLiteral("Line.Mode")));
    QCOMPARE(listing.locations.value(QStringLiteral("Motor.Load")), QStringLiteral("Motor"));
    QCOMPARE(listing.locations.value(QStringLiteral("Gearbox.Oil")), QStringLiteral("Motor/Gearbox"));

    const TreeNode *motor = VariableListing::find({ line }, QStringLiteral("Motor"));
    const VariableListing::Listing motorListing = VariableListing::collect(*motor, lineVariables(), true);
    QCOMPARE(motorListing.variables.size(), 3);
    QCOMPARE(motorListing.locations.value(QStringLiteral("Gearbox.Oil")), QStringLiteral("Gearbox"));
}

void TestTableModels::variableListingCollectsChildrenOfVariables()
{
    TreeNode motor = lineTree().children.at(1);
    motor.children.first().children = { treeNode(QStringLiteral("Speed.EURange"), TreeNode::Kind::Property) };
    QHash<QString, QList<VariableInfo>> variables = lineVariables();
    variables.insert(QStringLiteral("Motor.Speed"), { variable(QStringLiteral("Speed.EURange"), QStringLiteral("0; 100")) });

    const VariableListing::Listing nested = VariableListing::collect(motor, variables, true);
    QCOMPARE(nodeIds(nested.variables),
             (QStringList { QStringLiteral("Motor.Speed"), QStringLiteral("Motor.Load"), QStringLiteral("Speed.EURange"),
                            QStringLiteral("Gearbox.Oil") }));
    QCOMPARE(nested.locations.value(QStringLiteral("Speed.EURange")), QStringLiteral("Motor.Speed"));
    QCOMPARE(VariableListing::collect(motor, variables, false).variables.size(), 2);

    const TreeNode *speed = VariableListing::find({ motor }, QStringLiteral("Motor.Speed"));
    QCOMPARE(nodeIds(VariableListing::collect(*speed, variables, false).variables),
             QStringList { QStringLiteral("Speed.EURange") });
}

void TestTableModels::subscriptionsModel()
{
    SubscriptionInfo subscription;
    subscription.id = 7;
    subscription.publishingIntervalMs = 500.0;
    subscription.publishingEnabled = true;
    subscription.priority = 2;
    subscription.monitoredItems = 3;
    subscription.notifications = 40;
    subscription.maxKeepAliveCount = 10;
    subscription.maxLifetimeCount = 30;
    SubscriptionsModel model;
    model.setItems({ subscription });

    QCOMPARE(model.columnCount(), int(SubscriptionsModel::ColCount));
    QCOMPARE(header(model, SubscriptionsModel::ColPublishingInterval), QStringLiteral("Publishing Interval"));
    QCOMPARE(model.index(0, SubscriptionsModel::ColId).data().toUInt(), 7u);
    QCOMPARE(model.index(0, SubscriptionsModel::ColPublishingInterval).data().toString(), QStringLiteral("500 ms"));
    QCOMPARE(model.index(0, SubscriptionsModel::ColPublishing).data().toString(), QStringLiteral("Enabled"));
    QCOMPARE(model.index(0, SubscriptionsModel::ColMonitoredItems).data().toUInt(), 3u);
    QCOMPARE(model.index(0, SubscriptionsModel::ColNotifications).data().toUInt(), 40u);
    QCOMPARE(model.index(0, SubscriptionsModel::ColLifetime).data().toUInt(), 30u);
    QVERIFY(!model.index(0, SubscriptionsModel::ColId).data(Qt::DecorationRole).isValid());
}

void TestTableModels::tableModelsHaveNoChildren()
{
    LogModel model;
    model.setItems({ LogEntry{} });
    const QModelIndex cell = model.index(0, 0);
    QCOMPARE(model.rowCount(cell), 0);
    QCOMPARE(model.columnCount(cell), 0);
}

void TestTableModels::variablesModel()
{
    VariablesModel model;
    model.setItems({ variable(QStringLiteral("ns=2;s=Motor.Speed"), 1487.6),
                     variable(QStringLiteral("ns=2;s=Motor.Running"), true) });

    QCOMPARE(model.columnCount(), int(VariablesModel::ColCount));
    QCOMPARE(header(model, VariablesModel::ColName), QStringLiteral("Name"));
    QCOMPARE(header(model, VariablesModel::ColQuality), QStringLiteral("Quality"));
    QVERIFY(!model.headerData(0, Qt::Vertical).toString().contains(QStringLiteral("Name")));

    QCOMPARE(model.index(0, VariablesModel::ColName).data().toString(), QStringLiteral("Speed"));
    QCOMPARE(model.index(0, VariablesModel::ColName).data(ItemRoles::IconName).toString(),
             QStringLiteral("signal"));
    QVERIFY(!model.index(0, VariablesModel::ColValue).data(ItemRoles::IconName).isValid());
    QCOMPARE(model.index(0, VariablesModel::ColValue).data().toString(), QStringLiteral("1,487.6"));
    QCOMPARE(model.index(1, VariablesModel::ColValue).data().toString(), QStringLiteral("true"));
    QCOMPARE(model.index(0, VariablesModel::ColDataType).data().toString(), QStringLiteral("Double"));
    QCOMPARE(model.index(0, VariablesModel::ColValue).data(ItemRoles::Id).toString(),
             QStringLiteral("ns=2;s=Motor.Speed"));

    QCOMPARE(model.rowOf(QStringLiteral("ns=2;s=Motor.Running")), 1);
    QCOMPARE(model.rowOf(QStringLiteral("missing")), -1);
    VariableInfo plain = variable(QStringLiteral("plain"), 0);
    plain.icon.clear();
    plain.typeIcon = QStringLiteral("analog-type");
    model.setItems({ plain });
    QCOMPARE(model.index(0, VariablesModel::ColName).data(ItemRoles::IconName).toString(),
             QStringLiteral("analog-type"));
}

void TestTableModels::variablesModelQuality_data()
{
    QTest::addColumn<VariableInfo::Quality>("quality");
    QTest::addColumn<QString>("text");
    QTest::addColumn<StatusKind>("status");

    QTest::newRow("good") << VariableInfo::Quality::Good << QStringLiteral("Good") << StatusKind::Good;
    QTest::newRow("uncertain") << VariableInfo::Quality::Uncertain << QStringLiteral("Uncertain")
                               << StatusKind::Warning;
    QTest::newRow("bad") << VariableInfo::Quality::Bad << QStringLiteral("Bad") << StatusKind::Error;
    QTest::newRow("good local override") << VariableInfo::Quality::GoodLocalOverride
                                         << QStringLiteral("Good (local override)") << StatusKind::Good;
    QTest::newRow("uncertain last usable value") << VariableInfo::Quality::UncertainLastUsableValue
                                                 << QStringLiteral("Uncertain (last usable value)")
                                                 << StatusKind::Warning;
    QTest::newRow("bad sensor failure") << VariableInfo::Quality::BadSensorFailure
                                        << QStringLiteral("Bad (sensor failure)") << StatusKind::Error;
}

void TestTableModels::variablesModelQuality()
{
    QFETCH(VariableInfo::Quality, quality);
    QFETCH(QString, text);
    QFETCH(StatusKind, status);

    VariablesModel model;
    model.setItems({ variable(QStringLiteral("a.Speed"), 1.0, quality) });
    const QModelIndex cell = model.index(0, VariablesModel::ColQuality);
    QCOMPARE(cell.data().toString(), text);
    QCOMPARE(::status(cell), status);
    QCOMPARE(::status(model.index(0, VariablesModel::ColName)), StatusKind::None);
}

void TestTableModels::variablesModelPassesTester()
{
    VariablesModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.setItems({ variable(QStringLiteral("a.Speed"), 1.0) });
    model.clear();
}

void TestTableModels::subtypesModel()
{
    TypeInfo motor;
    motor.browseName = QStringLiteral("MotorType");
    motor.nodeId = QStringLiteral("ns=2;i=1001");
    motor.category = QStringLiteral("ObjectType");
    motor.children = 8;

    TypeInfo gauge;
    gauge.browseName = QStringLiteral("GaugeType");
    gauge.nodeId = QStringLiteral("ns=2;i=1002");
    gauge.isAbstract = true;

    TypeInfo servo;
    servo.browseName = QStringLiteral("ServoType");
    servo.nodeId = QStringLiteral("ns=2;i=1003");

    SubtypesModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.setSubtypes({ motor, servo, gauge }, { { servo.nodeId, motor.nodeId } },
                      { { motor.nodeId, QStringLiteral("object") } });
    QCOMPARE(model.rowCount(), 2);
    const QModelIndex motorIndex = model.index(0, SubtypesModel::ColName);
    QCOMPARE(model.rowCount(motorIndex), 1);
    QCOMPARE(model.rowCount(model.index(0, SubtypesModel::ColNodeId)), 0);
    const QModelIndex servoIndex = model.index(0, SubtypesModel::ColName, motorIndex);
    QCOMPARE(servoIndex.data().toString(), QStringLiteral("ServoType"));
    QCOMPARE(model.parent(servoIndex), motorIndex);
    QCOMPARE(model.indexForRow(model.rowOf(servo.nodeId)), servoIndex);
    QCOMPARE(model.columnCount(), int(SubtypesModel::ColCount));
    QCOMPARE(header(model, SubtypesModel::ColChildren), QStringLiteral("Children"));
    QCOMPARE(model.index(0, SubtypesModel::ColName).data().toString(), QStringLiteral("MotorType"));
    QCOMPARE(model.index(0, SubtypesModel::ColName).data(ItemRoles::IconName).toString(),
             QStringLiteral("object"));
    QVERIFY(!model.index(0, SubtypesModel::ColNodeId).data(ItemRoles::IconName).isValid());
    QVERIFY(model.index(1, SubtypesModel::ColName).data(ItemRoles::IconName).toString().isEmpty());
    QVERIFY(!model.index(0, SubtypesModel::ColName).data(ItemRoles::Muted).toBool());
    QVERIFY(model.index(1, SubtypesModel::ColName).data(ItemRoles::Muted).toBool());
    QVERIFY(model.index(1, SubtypesModel::ColChildren).data(ItemRoles::Muted).toBool());
    QCOMPARE(model.index(0, SubtypesModel::ColNodeId).data().toString(), QStringLiteral("ns=2;i=1001"));
    QCOMPARE(model.index(0, SubtypesModel::ColCategory).data().toString(), QStringLiteral("ObjectType"));
    QCOMPARE(model.index(0, SubtypesModel::ColChildren).data().toInt(), 8);
    QCOMPARE(model.index(0, SubtypesModel::ColChildren).data(ItemRoles::Id).toString(),
             QStringLiteral("ns=2;i=1001"));
}

void TestTableModels::referencesModel()
{
    ReferencesModel model;
    model.setItems({ { QStringLiteral("HasSubtype"), 3 }, { QStringLiteral("HasProperty"), 2 } });
    QCOMPARE(model.columnCount(), int(ReferencesModel::ColCount));
    QCOMPARE(header(model, ReferencesModel::ColReferences), QStringLiteral("Count"));
    QCOMPARE(model.index(0, ReferencesModel::ColReferenceType).data().toString(), QStringLiteral("HasSubtype"));
    QCOMPARE(model.index(1, ReferencesModel::ColReferences).data().toInt(), 2);
    QVERIFY(!model.index(1, ReferencesModel::ColReferences).data(Qt::DecorationRole).isValid());
}

void TestTableModels::namespacesModel()
{
    NamespacesModel model;
    model.setItems({ { 2, QStringLiteral("urn:factory"), QStringLiteral("1.0.0"), QDate(2024, 5, 20), {} } });
    QCOMPARE(model.columnCount(), int(NamespacesModel::ColCount));
    QCOMPARE(header(model, NamespacesModel::ColPublicationDate), QStringLiteral("Publication Date"));
    QCOMPARE(model.index(0, NamespacesModel::ColIndex).data().toInt(), 2);
    QCOMPARE(model.index(0, NamespacesModel::ColUri).data().toString(), QStringLiteral("urn:factory"));
    QCOMPARE(model.index(0, NamespacesModel::ColVersion).data().toString(), QStringLiteral("1.0.0"));
    QCOMPARE(model.index(0, NamespacesModel::ColPublicationDate).data().toString(), QStringLiteral("2024-05-20"));
    QCOMPARE(model.index(0, NamespacesModel::ColIndex).data(ItemRoles::Id).toString(), QStringLiteral("urn:factory"));
}

void TestTableModels::endpointsModel()
{
    EndpointInfo local;
    local.protocol = QStringLiteral("opc.tcp");
    local.hostname = QStringLiteral("localhost");
    local.port = 4840;
    local.path = QStringLiteral("/discovery");
    local.securityPolicy = QStringLiteral("None");
    local.securityMode = QStringLiteral("None");
    EndpointInfo disabled = local;
    disabled.path.clear();
    disabled.enabled = false;

    EndpointsModel model;
    model.setItems({ local, disabled });
    QCOMPARE(model.columnCount(), int(EndpointsModel::ColCount));
    QCOMPARE(header(model, EndpointsModel::ColSecurityPolicy), QStringLiteral("Security Policy"));
    QCOMPARE(model.index(0, EndpointsModel::ColUrl).data().toString(),
             QStringLiteral("opc.tcp://localhost:4840/discovery"));
    QCOMPARE(model.index(1, EndpointsModel::ColUrl).data().toString(), QStringLiteral("opc.tcp://localhost:4840"));
    QCOMPARE(model.index(0, EndpointsModel::ColProtocol).data().toString(), QStringLiteral("opc.tcp"));
    QCOMPARE(model.index(0, EndpointsModel::ColEnabled).data().toString(), QStringLiteral("Yes"));
    QCOMPARE(status(model.index(0, EndpointsModel::ColEnabled)), StatusKind::Good);
    QCOMPARE(model.index(1, EndpointsModel::ColEnabled).data().toString(), QStringLiteral("No"));
    QCOMPARE(status(model.index(1, EndpointsModel::ColEnabled)), StatusKind::Inactive);
    QCOMPARE(status(model.index(0, EndpointsModel::ColUrl)), StatusKind::None);
    QCOMPARE(header(model, EndpointsModel::ColUserAuthentication), QStringLiteral("User Authentication"));
    QCOMPARE(header(model, EndpointsModel::ColTrustedClientsOnly), QStringLiteral("Trusted Clients Only"));
    QCOMPARE(model.index(0, EndpointsModel::ColUserAuthentication).data().toString(), QStringLiteral("Anonymous"));
    QCOMPARE(model.index(0, EndpointsModel::ColTrustedClientsOnly).data().toString(), QStringLiteral("No"));
    QCOMPARE(status(model.index(0, EndpointsModel::ColTrustedClientsOnly)), StatusKind::None);
}

void TestTableModels::endpointUserAuthenticationText_data()
{
    QTest::addColumn<bool>("anonymous");
    QTest::addColumn<bool>("username");
    QTest::addColumn<bool>("certificate");
    QTest::addColumn<QString>("text");
    QTest::newRow("all") << true << true << true << QStringLiteral("Anonymous, Username, Certificate");
    QTest::newRow("username") << false << true << false << QStringLiteral("Username");
    QTest::newRow("certificate") << false << false << true << QStringLiteral("Certificate");
    QTest::newRow("none") << false << false << false << QStringLiteral("None");
}

void TestTableModels::endpointUserAuthenticationText()
{
    QFETCH(bool, anonymous);
    QFETCH(bool, username);
    QFETCH(bool, certificate);
    QFETCH(QString, text);
    EndpointInfo endpoint;
    endpoint.allowAnonymous = anonymous;
    endpoint.usernamePassword = username;
    endpoint.userCertificate = certificate;
    endpoint.trustedClientsOnly = true;

    EndpointsModel model;
    model.setItems({ endpoint });
    QCOMPARE(model.index(0, EndpointsModel::ColUserAuthentication).data().toString(), text);
    QCOMPARE(model.index(0, EndpointsModel::ColTrustedClientsOnly).data().toString(), QStringLiteral("Yes"));
}

void TestTableModels::certificatesModel()
{
    CertificateInfo certificate;
    certificate.category = CertificateInfo::Category::Trusted;
    certificate.commonName = QStringLiteral("Client A");
    certificate.serialNumber = QStringLiteral("01 23");
    certificate.validFrom = QDateTime(QDate(2024, 4, 10), QTime(10, 0, 30));
    certificate.validTo = QDateTime(QDate(2026, 4, 10), QTime(10, 0));
    certificate.fingerprint = QStringLiteral("3A:5F");

    CertificatesModel model;
    model.setItems({ certificate });
    QCOMPARE(model.columnCount(), int(CertificatesModel::ColCount));
    QCOMPARE(header(model, CertificatesModel::ColFingerprint), QStringLiteral("Fingerprint"));
    QCOMPARE(model.index(0, CertificatesModel::ColCommonName).data().toString(), QStringLiteral("Client A"));
    QCOMPARE(model.index(0, CertificatesModel::ColCommonName).data(ItemRoles::IconName).toString(),
             QStringLiteral("certificate"));
    QCOMPARE(model.index(0, CertificatesModel::ColValidFrom).data().toString(), QStringLiteral("2024-04-10 10:00"));
    QCOMPARE(model.index(0, CertificatesModel::ColValidTo).data().toString(), QStringLiteral("2026-04-10 10:00"));
    QCOMPARE(model.index(0, CertificatesModel::ColStatus).data().toString(), QStringLiteral("Trusted"));
    QCOMPARE(status(model.index(0, CertificatesModel::ColStatus)), StatusKind::Good);
    QCOMPARE(model.index(0, CertificatesModel::ColFingerprint).data().toString(), QStringLiteral("3A:5F"));
    QCOMPARE(model.index(0, CertificatesModel::ColFingerprint).data(Qt::ToolTipRole).toString(), QStringLiteral("3A:5F"));
    QVERIFY(!model.index(0, CertificatesModel::ColCommonName).data(Qt::ToolTipRole).isValid());
    QCOMPARE(model.index(0, CertificatesModel::ColStatus).data(ItemRoles::Id).toString(), QStringLiteral("01 23"));
}

void TestTableModels::certificateStatus_data()
{
    QTest::addColumn<CertificateInfo::Category>("category");
    QTest::addColumn<QString>("text");
    QTest::addColumn<StatusKind>("status");

    QTest::newRow("application") << CertificateInfo::Category::Application << QStringLiteral("Own") << StatusKind::Good;
    QTest::newRow("trusted") << CertificateInfo::Category::Trusted << QStringLiteral("Trusted") << StatusKind::Good;
    QTest::newRow("rejected") << CertificateInfo::Category::Rejected << QStringLiteral("Rejected") << StatusKind::Error;
    QTest::newRow("issuer") << CertificateInfo::Category::Issuer << QStringLiteral("Issuer") << StatusKind::Info;
}

void TestTableModels::certificateStatus()
{
    QFETCH(CertificateInfo::Category, category);
    QFETCH(QString, text);
    QFETCH(StatusKind, status);
    QCOMPARE(CertificatesModel::statusText(category), text);
    QCOMPARE(CertificatesModel::status(category), status);
}

void TestTableModels::sessionsModel()
{
    SessionInfo active;
    active.sessionId = QStringLiteral("ns=1;i=1");
    active.name = QStringLiteral("UaExpert");
    active.client = QStringLiteral("UaExpert/1.6.4");
    active.user = QStringLiteral("anonymous");
    active.active = true;
    active.subscriptions = 3;
    active.lastActivity = QDateTime(QDate(2024, 5, 23), QTime(14, 31, 52, 4));
    SessionInfo closed = active;
    closed.sessionId = QStringLiteral("ns=1;i=2");
    closed.active = false;

    SessionsModel model;
    model.setItems({ active, closed });
    QCOMPARE(model.columnCount(), int(SessionsModel::ColCount));
    QCOMPARE(header(model, SessionsModel::ColLastActivity), QStringLiteral("Last Activity"));
    QCOMPARE(model.index(0, SessionsModel::ColName).data(ItemRoles::IconName).toString(), QStringLiteral("user"));
    QCOMPARE(model.index(0, SessionsModel::ColClient).data().toString(), QStringLiteral("UaExpert/1.6.4"));
    QCOMPARE(model.index(0, SessionsModel::ColUser).data().toString(), QStringLiteral("anonymous"));
    QCOMPARE(model.index(0, SessionsModel::ColState).data().toString(), QStringLiteral("Active"));
    QCOMPARE(status(model.index(0, SessionsModel::ColState)), StatusKind::Good);
    QCOMPARE(model.index(1, SessionsModel::ColState).data().toString(), QStringLiteral("Closed"));
    QCOMPARE(status(model.index(1, SessionsModel::ColState)), StatusKind::Inactive);
    QCOMPARE(model.index(0, SessionsModel::ColSubscriptions).data().toInt(), 3);
    QCOMPARE(model.index(0, SessionsModel::ColLastActivity).data().toString(),
             QStringLiteral("2024-05-23 14:31:52"));
    QCOMPARE(model.rowOf(QStringLiteral("ns=1;i=2")), 1);
    QCOMPARE(model.rowOf(QStringLiteral("ns=1;i=9")), -1);
}

void TestTableModels::logModel()
{
    LogEntry entry;
    entry.time = QDateTime(QDate(2024, 5, 23), QTime(14, 31, 20, 4));
    entry.level = LogEntry::Level::Warning;
    entry.category = LogEntry::Category::Server;
    entry.message = QStringLiteral("Slow client detected");

    LogModel model;
    model.setItems({ entry });
    QCOMPARE(model.columnCount(), int(LogModel::ColCount));
    QCOMPARE(header(model, LogModel::ColCategory), QStringLiteral("Category"));
    QCOMPARE(model.index(0, LogModel::ColTime).data().toString(), QStringLiteral("2024-05-23 14:31:20.004"));
    QCOMPARE(model.index(0, LogModel::ColLevel).data().toString(), QStringLiteral("WARNING"));
    QCOMPARE(status(model.index(0, LogModel::ColLevel)), StatusKind::Warning);
    QCOMPARE(status(model.index(0, LogModel::ColMessage)), StatusKind::None);
    QCOMPARE(model.index(0, LogModel::ColCategory).data().toString(), QStringLiteral("Server"));
    QCOMPARE(model.index(0, LogModel::ColMessage).data().toString(), QStringLiteral("Slow client detected"));
}

void TestTableModels::logLevels_data()
{
    QTest::addColumn<LogEntry::Level>("level");
    QTest::addColumn<QString>("text");
    QTest::addColumn<StatusKind>("status");

    QTest::newRow("info") << LogEntry::Level::Info << QStringLiteral("INFO") << StatusKind::Info;
    QTest::newRow("warning") << LogEntry::Level::Warning << QStringLiteral("WARNING") << StatusKind::Warning;
    QTest::newRow("error") << LogEntry::Level::Error << QStringLiteral("ERROR") << StatusKind::Error;
}

void TestTableModels::logLevels()
{
    QFETCH(LogEntry::Level, level);
    QFETCH(QString, text);
    QFETCH(StatusKind, status);
    QCOMPARE(LogModel::levelText(level), text);
    QCOMPARE(LogModel::levelStatus(level), status);
}

void TestTableModels::logCategories()
{
    QCOMPARE(LogModel::categoryText(LogEntry::Category::Server), QStringLiteral("Server"));
    QCOMPARE(LogModel::categoryText(LogEntry::Category::Session), QStringLiteral("Session"));
    QCOMPARE(LogModel::categoryText(LogEntry::Category::Security), QStringLiteral("Security"));
    QCOMPARE(LogModel::categoryText(LogEntry::Category::DataChanges), QStringLiteral("Data Changes"));
}

QTEST_GUILESS_MAIN(TestTableModels)
#include "test_tablemodels.moc"
