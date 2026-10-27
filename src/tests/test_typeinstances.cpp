// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QTest>

#include <QUaServer>

#include <algorithm>

#include "project/portablenodeid.h"
#include "project/projectfile.h"
#include "project/projectnodes.h"
#include "server/declarednodes.h"
#include "server/servercontroller.h"
#include "server/typecatalog.h"
#include "testport.h"
#include "testproject.h"

namespace {

const QString kPumpUri = QStringLiteral("urn:openuasim:test:pump");
const QString kPumpType = QStringLiteral("nsu=urn:openuasim:test:pump;i=1000");
const QString kPump = QStringLiteral("ns=1;s=Pump1");
const QString kSpeed = QStringLiteral("ns=1;s=Pump1.Speed");
const QString kDiagnostics = QStringLiteral("ns=1;s=Pump1.Diagnostics");
const QString kTemperature = QStringLiteral("ns=1;s=Pump1.Diagnostics.Temperature");

QString testData(const QString &name)
{
    return QStringLiteral(OUASIM_TEST_DATA_DIR "/") + name;
}

ProjectConfig pumpProject()
{
    ProjectConfig project = TestProject::create();
    project.endpoints.first().port = TestPort::freePort();
    project.nodeSetFiles = { testData(QStringLiteral("pump.NodeSet2.xml")) };
    return project;
}

NodeDefinition typedNode(NodeDefinition::Kind kind, const QString &name, const QString &typeDefinition)
{
    NodeDefinition node;
    node.kind = kind;
    node.browseName = name;
    node.typeDefinition = typeDefinition;
    return node;
}

const TreeNode *findTreeNode(const QList<TreeNode> &nodes, const QString &id)
{
    for (const TreeNode &node : nodes) {
        if (node.id == id)
            return &node;
        if (const TreeNode *found = findTreeNode(node.children, id))
            return found;
    }
    return nullptr;
}

QStringList childNames(const TreeNode &node)
{
    QStringList names;
    for (const TreeNode &child : node.children)
        names.append(child.text);
    names.sort();
    return names;
}

VariableInfo variable(const ServerController &controller, const QString &ownerId, const QString &nodeId)
{
    const QList<VariableInfo> variables = controller.snapshot().variables.value(ownerId);
    const auto found = std::find_if(variables.cbegin(), variables.cend(),
                                    [&nodeId](const VariableInfo &info) { return info.nodeId == nodeId; });
    return found != variables.cend() ? *found : VariableInfo();
}

QStringList typeNames(const QList<TypeInfo> &types)
{
    QStringList names;
    for (const TypeInfo &type : types)
        names.append(type.browseName);
    return names;
}

PropertyInfo findProperty(const VariableInfo &variable, const QString &name)
{
    for (const PropertyInfo &property : variable.properties) {
        if (property.browseName == name)
            return property;
    }
    return {};
}

VariableInfo withEditedProperty(VariableInfo variable, const QString &name, const QString &value)
{
    for (PropertyInfo &property : variable.properties) {
        if (property.browseName == name) {
            property.value = value;
            property.edited = true;
        }
    }
    return variable;
}

const QString kTank = QStringLiteral("ns=1;s=Tank");

QString addLevel(ServerController &controller)
{
    NodeDefinition tank;
    tank.browseName = QStringLiteral("Tank");
    controller.addNode(QString(), tank);
    NodeDefinition level = typedNode(NodeDefinition::Kind::Variable, QStringLiteral("Level"), QStringLiteral("i=2368"));
    level.dataType = QStringLiteral("Double");
    return controller.addNode(kTank, level);
}

bool logContains(const QList<LogEntry> &log, const QString &text)
{
    return std::any_of(log.cbegin(), log.cend(), [&text](const LogEntry &entry) { return entry.message.contains(text); });
}

}

class TestTypeInstances : public QObject
{
    Q_OBJECT

private slots:
    void portableNodeIdsNameNamespacesByUri();
    void listsInstanceTypesPerKind();
    void instanceGetsChildrenOfItsType();
    void editingDeclaredVariableKeepsSettings();
    void declaredSettingsSurviveReload();
    void copiedInstanceKeepsItsSettings();
    void typedVariableChecksDataType();
    void unloadedTypeIsLogged();
    void rejectsTypeOfOtherKind();
    void projectFileKeepsTypeAndDeclaredNodes();
    void variablesShowTheirTypeAndProperties();
    void editedPropertiesAreKeptAndServed();
    void invalidPropertyValueIsRefused();
    void modelVariableKeepsEditedProperties();
    void propertyNodeEditsItsVariable();
};

void TestTypeInstances::portableNodeIdsNameNamespacesByUri()
{
    const QStringList namespaces { QStringLiteral("http://opcfoundation.org/UA/"), QStringLiteral("urn:app"), kPumpUri };
    QCOMPARE(PortableNodeId::fromServer(QStringLiteral("ns=2;i=1000"), namespaces), kPumpType);
    QCOMPARE(PortableNodeId::fromServer(QStringLiteral("i=2368"), namespaces), QStringLiteral("i=2368"));
    QVERIFY(PortableNodeId::fromServer(QStringLiteral("ns=5;i=1"), namespaces).isEmpty());
    QVERIFY(PortableNodeId::fromServer(QStringLiteral("garbage"), namespaces).isEmpty());

    QCOMPARE(PortableNodeId::toServer(kPumpType, namespaces), QStringLiteral("ns=2;i=1000"));
    QCOMPARE(PortableNodeId::toServer(QStringLiteral("i=2368"), namespaces), QStringLiteral("i=2368"));
    QCOMPARE(PortableNodeId::toServer(QStringLiteral("nsu=http://opcfoundation.org/UA/;i=58"), namespaces),
             QStringLiteral("i=58"));
    QVERIFY(PortableNodeId::toServer(QStringLiteral("nsu=urn:missing;i=1"), namespaces).isEmpty());
    QVERIFY(PortableNodeId::toServer(QStringLiteral("nsu=urn:app;x"), namespaces).isEmpty());
}

void TestTypeInstances::listsInstanceTypesPerKind()
{
    ServerController controller;
    controller.setProject(pumpProject());
    const QHash<QString, TypeInfo> types = controller.snapshot().types;

    const QStringList objects = typeNames(TypeCatalog::instanceTypes(types, NodeDefinition::Kind::Object));
    QCOMPARE(objects.first(), QStringLiteral("BaseObjectType"));
    QVERIFY(objects.contains(QStringLiteral("PumpType")));
    QVERIFY2(!objects.contains(QStringLiteral("MachineType")), "abstract types are left out");
    QVERIFY2(!objects.contains(QStringLiteral("CabinetType")), "folder types belong to folders");
    QVERIFY2(!objects.contains(QStringLiteral("ServerType")), "standard object types are left out");

    const QStringList folders = typeNames(TypeCatalog::instanceTypes(types, NodeDefinition::Kind::Folder));
    QCOMPARE(folders, QStringList({ QStringLiteral("FolderType"), QStringLiteral("CabinetType") }));

    const QStringList variables = typeNames(TypeCatalog::instanceTypes(types, NodeDefinition::Kind::Variable));
    QCOMPARE(variables.first(), QStringLiteral("BaseDataVariableType"));
    QVERIFY(variables.contains(QStringLiteral("AnalogItemType")));
    QVERIFY(!variables.contains(QStringLiteral("PropertyType")));

    QVERIFY(TypeCatalog::instanceTypes(types, NodeDefinition::Kind::Method).isEmpty());
}

void TestTypeInstances::instanceGetsChildrenOfItsType()
{
    ServerController controller;
    controller.setProject(pumpProject());
    QString error;
    QCOMPARE(controller.addNode(QString(), typedNode(NodeDefinition::Kind::Object, QStringLiteral("Pump1"), kPumpType),
                                &error),
             kPump);

    auto *pump = controller.server().uaServer()->nodeById<QUaBaseObject>(QUaNodeId(kPump));
    QVERIFY(pump);
    QCOMPARE(pump->typeDefinitionBrowseName().name(), QStringLiteral("PumpType"));

    const ServerSnapshot snapshot = controller.snapshot();
    const TreeNode *node = findTreeNode(snapshot.addressSpace, kPump);
    QVERIFY(node);
    QCOMPARE(childNames(*node), QStringList({ QStringLiteral("Diagnostics"), QStringLiteral("Recipe"),
                                              QStringLiteral("Speed") }));
    const TreeNode *diagnostics = findTreeNode(snapshot.addressSpace, kDiagnostics);
    QVERIFY(diagnostics);
    QCOMPARE(childNames(*diagnostics), QStringList({ QStringLiteral("Temperature") }));

    const VariableInfo speed = variable(controller, kPump, kSpeed);
    QCOMPARE(speed.value.toDouble(), 1450.0);
    QVERIFY(speed.fromModel);
    QVERIFY(!speed.readOnly);
    const VariableInfo recipe = variable(controller, kPump, QStringLiteral("ns=1;s=Pump1.Recipe"));
    QVERIFY2(recipe.readOnly, "the simulation cannot drive NodeId values");
    QCOMPARE(variable(controller, kDiagnostics, kTemperature).value.toDouble(), 35.0);
}

void TestTypeInstances::editingDeclaredVariableKeepsSettings()
{
    ServerController controller;
    controller.setProject(pumpProject());
    controller.addNode(QString(), typedNode(NodeDefinition::Kind::Object, QStringLiteral("Pump1"), kPumpType));
    VariableInfo temperature = variable(controller, kDiagnostics, kTemperature);
    temperature.initialValue = QStringLiteral("60");
    temperature.icon = QStringLiteral("thermometer");

    QString error;
    QVERIFY2(controller.updateVariable(temperature, &error), qPrintable(error));
    const NodeDefinition *diagnostics = ProjectNodes::find(controller.project().nodes, kDiagnostics);
    QVERIFY(diagnostics);
    QVERIFY(diagnostics->declared);
    const NodeDefinition *stored = ProjectNodes::find(controller.project().nodes, kTemperature);
    QVERIFY(stored);
    QVERIFY(stored->declared);
    QCOMPARE(stored->browseName, QStringLiteral("Temperature"));
    QCOMPARE(stored->dataType, QStringLiteral("Double"));
    QCOMPARE(stored->icon, QStringLiteral("thermometer"));

    const VariableInfo shown = variable(controller, kDiagnostics, kTemperature);
    QCOMPARE(shown.value.toDouble(), 60.0);
    QVERIFY(shown.fromModel);
    const ServerSnapshot snapshot = controller.snapshot();
    const TreeNode *pump = findTreeNode(snapshot.addressSpace, kPump);
    QVERIFY(pump);
    QCOMPARE(pump->children.size(), 3);

    temperature = shown;
    temperature.initialValue = QStringLiteral("70");
    QVERIFY2(controller.updateVariable(temperature, &error), qPrintable(error));
    QCOMPARE(variable(controller, kDiagnostics, kTemperature).value.toDouble(), 70.0);
    QCOMPARE(DeclaredNodes::settings(controller.project().nodes).size(), 1);
}

void TestTypeInstances::declaredSettingsSurviveReload()
{
    ServerController editor;
    editor.setProject(pumpProject());
    editor.addNode(QString(), typedNode(NodeDefinition::Kind::Object, QStringLiteral("Pump1"), kPumpType));
    VariableInfo speed = variable(editor, kPump, kSpeed);
    speed.initialValue = QStringLiteral("900");
    speed.configuredQuality = VariableInfo::Quality::Uncertain;
    QVERIFY(editor.updateVariable(speed));

    ServerController controller;
    controller.setProject(editor.project());
    const VariableInfo loaded = variable(controller, kPump, kSpeed);
    QCOMPARE(loaded.value.toDouble(), 900.0);
    QCOMPARE(loaded.quality, VariableInfo::Quality::Uncertain);
    QVERIFY(controller.addressSpace().hasVariable(kSpeed));
    QVERIFY(!logContains(controller.log(), QStringLiteral("Node skipped")));
}

void TestTypeInstances::copiedInstanceKeepsItsSettings()
{
    ServerController controller;
    controller.setProject(pumpProject());
    controller.addNode(QString(), typedNode(NodeDefinition::Kind::Object, QStringLiteral("Pump1"), kPumpType));
    VariableInfo speed = variable(controller, kPump, kSpeed);
    speed.initialValue = QStringLiteral("900");
    QVERIFY(controller.updateVariable(speed));

    QString error;
    const QString copyId = controller.copyNode(kPump, QString(), &error);
    QVERIFY2(!copyId.isEmpty(), qPrintable(error));
    const NodeDefinition *copy = ProjectNodes::find(controller.project().nodes, copyId);
    QVERIFY(copy);
    QCOMPARE(copy->typeDefinition, kPumpType);
    QCOMPARE(copy->children.size(), 1);
    const QString copiedSpeed = copy->children.first().nodeId;
    QVERIFY(copiedSpeed != kSpeed);
    QCOMPARE(variable(controller, copyId, copiedSpeed).value.toDouble(), 900.0);
    QVERIFY(!logContains(controller.log(), QStringLiteral("Node skipped")));
}

void TestTypeInstances::typedVariableChecksDataType()
{
    ServerController controller;
    controller.setProject(pumpProject());
    NodeDefinition level = typedNode(NodeDefinition::Kind::Variable, QStringLiteral("Level"), QStringLiteral("i=2368"));
    level.dataType = QStringLiteral("Double");
    level.initialValue = 12.5;
    const QString levelId = controller.addNode(QString(), level);
    QVERIFY(controller.addressSpace().hasVariable(levelId));
    auto *built = controller.server().uaServer()->nodeById<QUaBaseDataVariable>(QUaNodeId(levelId));
    QVERIFY(built);
    QCOMPARE(built->typeDefinitionBrowseName().name(), QStringLiteral("AnalogItemType"));
    QCOMPARE(built->value().toDouble(), 12.5);
    QVERIFY(built->browseChild(QUaQualifiedName(0, QStringLiteral("EURange"))));

    NodeDefinition text = typedNode(NodeDefinition::Kind::Variable, QStringLiteral("Text"), QStringLiteral("i=2368"));
    text.dataType = QStringLiteral("String");
    QString error;
    const QString textId = controller.addNode(QString(), text, &error);
    QVERIFY(textId.isEmpty());
    QVERIFY(error.contains(QStringLiteral("does not accept String")));
}

void TestTypeInstances::unloadedTypeIsLogged()
{
    ServerController controller;
    controller.setProject(pumpProject());
    const QString id = controller.addNode(
        QString(), typedNode(NodeDefinition::Kind::Object, QStringLiteral("Ghost"), QStringLiteral("nsu=urn:missing;i=1")));
    QVERIFY(!id.isEmpty());
    QVERIFY(!controller.server().uaServer()->nodeById(QUaNodeId(id)));
    QVERIFY(logContains(controller.log(), QStringLiteral("type 'nsu=urn:missing;i=1' is not loaded")));
}

///
/// \brief A type of another node class is refused before the project is changed.
///
void TestTypeInstances::rejectsTypeOfOtherKind()
{
    ServerController controller;
    controller.setProject(pumpProject());
    QString error;
    const QString id = controller.addNode(
        QString(), typedNode(NodeDefinition::Kind::Variable, QStringLiteral("Odd"), kPumpType), &error);
    QVERIFY(id.isEmpty());
    QVERIFY(error.contains(QStringLiteral("not a data variable type")));
}

void TestTypeInstances::projectFileKeepsTypeAndDeclaredNodes()
{
    ProjectConfig project = pumpProject();
    NodeDefinition pump = typedNode(NodeDefinition::Kind::Object, QStringLiteral("Pump1"), kPumpType);
    pump.nodeId = kPump;
    NodeDefinition speed;
    speed.kind = NodeDefinition::Kind::Variable;
    speed.browseName = QStringLiteral("Speed");
    speed.nodeId = kSpeed;
    speed.declared = true;
    pump.children = { speed };
    project.nodes = { pump };

    const ProjectFile::LoadResult loaded = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(loaded.ok(), qPrintable(loaded.error));
    const NodeDefinition &read = loaded.project.nodes.first();
    QCOMPARE(read.typeDefinition, kPumpType);
    QVERIFY(!read.declared);
    QVERIFY(read.children.first().declared);
    QVERIFY(read.children.first().typeDefinition.isEmpty());

    NodeDefinition level = typedNode(NodeDefinition::Kind::Variable, QStringLiteral("Level"), QStringLiteral("i=2368"));
    level.nodeId = QStringLiteral("ns=1;s=Level");
    level.properties = { { QStringLiteral("EURange"), QStringLiteral("0; 100") },
                         { QStringLiteral("EngineeringUnits"), QStringLiteral("m") } };
    project.nodes.append(level);
    const ProjectFile::LoadResult withProperties = ProjectFile::fromJson(ProjectFile::toJson(project));
    QVERIFY2(withProperties.ok(), qPrintable(withProperties.error));
    QCOMPARE(withProperties.project.nodes.last().properties, level.properties);
    QVERIFY(withProperties.project.nodes.first().properties.isEmpty());
}

void TestTypeInstances::variablesShowTheirTypeAndProperties()
{
    ServerController controller;
    controller.setProject(pumpProject());
    controller.addNode(QString(), typedNode(NodeDefinition::Kind::Object, QStringLiteral("Pump1"), kPumpType));
    const QString levelId = addLevel(controller);
    NodeDefinition plain;
    plain.kind = NodeDefinition::Kind::Variable;
    plain.browseName = QStringLiteral("Plain");
    const QString plainId = controller.addNode(kTank, plain);

    const ServerSnapshot snapshot = controller.snapshot();
    const TreeNode *range = findTreeNode(snapshot.addressSpace, QStringLiteral("ns=1;s=Tank.Level.EURange"));
    QVERIFY(range);
    QCOMPARE(range->text, QStringLiteral("EURange"));
    QCOMPARE(range->kind, TreeNode::Kind::Property);
    QCOMPARE(range->icon, QStringLiteral("property"));
    const QStringList analogProperties { QStringLiteral("Definition"), QStringLiteral("EURange"),
                                         QStringLiteral("EngineeringUnits"), QStringLiteral("InstrumentRange"),
                                         QStringLiteral("ValuePrecision") };
    QCOMPARE(childNames(*findTreeNode(snapshot.addressSpace, levelId)), analogProperties);
    QVERIFY(findTreeNode(snapshot.addressSpace, plainId)->children.isEmpty());
    const QList<VariableInfo> levelChildren = snapshot.variables.value(levelId);
    QCOMPARE(levelChildren.size(), analogProperties.size());
    const VariableInfo &rangeInfo = levelChildren.first();
    QCOMPARE(rangeInfo.nodeId, QStringLiteral("ns=1;s=Tank.Level.EURange"));
    QCOMPARE(rangeInfo.dataType, QStringLiteral("Range"));
    QCOMPARE(rangeInfo.value.toString(), QStringLiteral("0; 0"));
    QCOMPARE(rangeInfo.typeName, QStringLiteral("PropertyType"));
    QCOMPARE(rangeInfo.icon, QStringLiteral("property"));
    QVERIFY(rangeInfo.property);
    QVERIFY(!rangeInfo.readOnly);
    const auto units = std::find_if(levelChildren.cbegin(), levelChildren.cend(), [](const VariableInfo &info) {
        return info.displayName == QStringLiteral("EngineeringUnits");
    });
    QVERIFY(units != levelChildren.cend());
    QCOMPARE(units->nodeId, QStringLiteral("ns=1;s=Tank.Level.EngineeringUnits"));
    QCOMPARE(units->dataType, QStringLiteral("EUInformation"));
    auto *servedRange = controller.server().uaServer()->nodeById<QUaBaseVariable>(QUaNodeId(rangeInfo.nodeId));
    QVERIFY(servedRange);
    QCOMPARE(servedRange->dataTypeNodeId(), QStringLiteral("ns=0;i=884"));

    const VariableInfo level = variable(controller, kTank, levelId);
    QVERIFY(level.icon.isEmpty());
    QCOMPARE(level.typeIcon, TypeCatalog::typeIconName(snapshot.types, QStringLiteral("i=2368")));
    QCOMPARE(findTreeNode(snapshot.addressSpace, levelId)->icon, level.typeIcon);
    QCOMPARE(level.typeName, QStringLiteral("AnalogItemType"));
    QCOMPARE(findProperty(level, QStringLiteral("EURange")).dataType, QStringLiteral("Range"));
    QVERIFY(findProperty(level, QStringLiteral("EngineeringUnits")).editable);
    const VariableInfo plainInfo = variable(controller, kTank, plainId);
    QCOMPARE(plainInfo.typeIcon, TypeCatalog::typeIconName(snapshot.types, QStringLiteral("i=63")));
    QCOMPARE(findTreeNode(snapshot.addressSpace, plainId)->icon, plainInfo.typeIcon);
    QCOMPARE(plainInfo.typeName, QStringLiteral("BaseDataVariableType"));
    QVERIFY(plainInfo.properties.isEmpty());
    QCOMPARE(variable(controller, kPump, kSpeed).typeName, QStringLiteral("BaseDataVariableType"));
}

void TestTypeInstances::editedPropertiesAreKeptAndServed()
{
    ServerController controller;
    controller.setProject(pumpProject());
    const QString levelId = addLevel(controller);
    VariableInfo level = variable(controller, kTank, levelId);
    level = withEditedProperty(level, QStringLiteral("EURange"), QStringLiteral(" 0.50 ;1e2"));
    level = withEditedProperty(level, QStringLiteral("EngineeringUnits"), QStringLiteral("m³"));

    QString error;
    QVERIFY2(controller.updateVariable(level, &error), qPrintable(error));
    const NodeDefinition *stored = ProjectNodes::find(controller.project().nodes, levelId);
    QVERIFY(stored);
    QCOMPARE(stored->properties.value(QStringLiteral("EngineeringUnits")), QStringLiteral("m³"));
    QCOMPARE(stored->properties.value(QStringLiteral("EURange")), QStringLiteral("0.5; 100"));

    auto *served = controller.server().uaServer()->nodeById<QUaBaseDataVariable>(QUaNodeId(levelId));
    QVERIFY(served);
    QVERIFY(served->browseChild(QUaQualifiedName(0, QStringLiteral("EngineeringUnits"))));
    QCOMPARE(childNames(*findTreeNode(controller.snapshot().addressSpace, levelId)).size(), 5);
    QCOMPARE(findProperty(variable(controller, kTank, levelId), QStringLiteral("EngineeringUnits")).value,
             QStringLiteral("m³"));

    ServerController reloaded;
    reloaded.setProject(controller.project());
    QCOMPARE(findProperty(variable(reloaded, kTank, levelId), QStringLiteral("EngineeringUnits")).value,
             QStringLiteral("m³"));
    QVERIFY(!logContains(reloaded.log(), QStringLiteral("Node skipped")));

    level = withEditedProperty(variable(controller, kTank, levelId), QStringLiteral("EngineeringUnits"),
                               QString());
    QVERIFY2(controller.updateVariable(level, &error), qPrintable(error));
    stored = ProjectNodes::find(controller.project().nodes, levelId);
    QVERIFY(!stored->properties.contains(QStringLiteral("EngineeringUnits")));
    QVERIFY(stored->properties.contains(QStringLiteral("EURange")));
    QVERIFY(findProperty(variable(controller, kTank, levelId), QStringLiteral("EngineeringUnits"))
                .value.isEmpty());
}

void TestTypeInstances::invalidPropertyValueIsRefused()
{
    ServerController controller;
    controller.setProject(pumpProject());
    const QString levelId = addLevel(controller);
    const VariableInfo level =
        withEditedProperty(variable(controller, kTank, levelId), QStringLiteral("EURange"),
                           QStringLiteral("100"));

    QString error;
    QVERIFY(!controller.updateVariable(level, &error));
    QCOMPARE(error, QStringLiteral("'100' is not a Range value for 'EURange'"));
    QVERIFY(ProjectNodes::find(controller.project().nodes, levelId)->properties.isEmpty());
}

void TestTypeInstances::modelVariableKeepsEditedProperties()
{
    const QString station = QStringLiteral("ns=2;i=4000");
    const QString pressure = QStringLiteral("ns=2;i=4001");
    ServerController controller;
    controller.setProject(pumpProject());
    VariableInfo info = variable(controller, station, pressure);
    QCOMPARE(info.typeName, QStringLiteral("AnalogItemType"));
    QVERIFY(info.icon.isEmpty());
    QCOMPARE(info.typeIcon, QStringLiteral("analog-type"));
    const ServerSnapshot iconsSnapshot = controller.snapshot();
    QCOMPARE(findTreeNode(iconsSnapshot.addressSpace, pressure)->icon, info.typeIcon);
    QCOMPARE(info.properties.size(), 5);
    QCOMPARE(findProperty(info, QStringLiteral("EURange")).value, QStringLiteral("0; 10"));
    QVERIFY(findProperty(info, QStringLiteral("EURange")).editable);

    info = withEditedProperty(info, QStringLiteral("EURange"), QStringLiteral("0; 16"));
    QString error;
    QVERIFY2(controller.updateVariable(info, &error), qPrintable(error));
    QCOMPARE(controller.project().modelVariables.size(), 1);
    const QMap<QString, QString> expected { { QStringLiteral("EURange"), QStringLiteral("0; 16") } };
    QCOMPARE(controller.project().modelVariables.first().variable.properties, expected);
    QCOMPARE(findProperty(variable(controller, station, pressure), QStringLiteral("EURange")).value,
             QStringLiteral("0; 16"));

    info = variable(controller, station, pressure);
    info.simulation.function = SimulationFunction::Sine;
    QVERIFY2(controller.updateVariable(info, &error), qPrintable(error));
    QCOMPARE(controller.project().modelVariables.first().variable.properties, expected);

    ServerController reloaded;
    reloaded.setProject(controller.project());
    QCOMPARE(findProperty(variable(reloaded, station, pressure), QStringLiteral("EURange")).value,
             QStringLiteral("0; 16"));
    QVERIFY(!logContains(reloaded.log(), QStringLiteral("skipped")));
}

void TestTypeInstances::propertyNodeEditsItsVariable()
{
    ServerController controller;
    controller.setProject(pumpProject());
    const QString levelId = addLevel(controller);
    const QString rangeId = QStringLiteral("ns=1;s=Tank.Level.EURange");
    VariableInfo range = variable(controller, levelId, rangeId);
    QVERIFY(range.property);
    QVERIFY(!range.readOnly);
    QCOMPARE(range.initialValue.toString(), QStringLiteral("0; 0"));

    range.initialValue = QStringLiteral("0; 100");
    QString error;
    QVERIFY2(controller.updateVariable(range, &error), qPrintable(error));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, levelId)->properties.value(QStringLiteral("EURange")),
             QStringLiteral("0; 100"));
    QCOMPARE(variable(controller, levelId, rangeId).value.toString(), QStringLiteral("0; 100"));

    range = variable(controller, levelId, rangeId);
    range.initialValue = QStringLiteral("high");
    QVERIFY(!controller.updateVariable(range, &error));
    QCOMPARE(error, QStringLiteral("'high' is not a Range value for 'EURange'"));

    VariableInfo stray = range;
    stray.nodeId = QStringLiteral("ns=1;s=Nowhere");
    QVERIFY(!controller.updateVariable(stray, &error));
    QCOMPARE(error, QStringLiteral("'ns=1;s=Nowhere' is not a property of a variable"));

    const QString pressure = QStringLiteral("ns=2;i=4001");
    VariableInfo modelRange = variable(controller, QStringLiteral("ns=2;i=4000"), pressure);
    QVERIFY(!modelRange.nodeId.isEmpty());
    const QList<VariableInfo> pressureChildren = controller.snapshot().variables.value(pressure);
    QCOMPARE(pressureChildren.size(), 1);
    modelRange = pressureChildren.first();
    modelRange.initialValue = QStringLiteral("0; 25");
    QVERIFY2(controller.updateVariable(modelRange, &error), qPrintable(error));
    QCOMPARE(controller.project().modelVariables.first().variable.properties.value(QStringLiteral("EURange")),
             QStringLiteral("0; 25"));
}

QTEST_GUILESS_MAIN(TestTypeInstances)

#include "test_typeinstances.moc"
