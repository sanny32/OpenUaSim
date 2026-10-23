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
    void typeOfOtherKindIsLogged();
    void projectFileKeepsTypeAndDeclaredNodes();
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
    const TreeNode *pump = findTreeNode(controller.snapshot().addressSpace, kPump);
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
    const QString textId = controller.addNode(QString(), text);
    QVERIFY(!controller.addressSpace().hasVariable(textId));
    QVERIFY(logContains(controller.log(), QStringLiteral("type 'i=2368' takes no String values")));
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

void TestTypeInstances::typeOfOtherKindIsLogged()
{
    ServerController controller;
    controller.setProject(pumpProject());
    const QString id =
        controller.addNode(QString(), typedNode(NodeDefinition::Kind::Variable, QStringLiteral("Odd"), kPumpType));
    QVERIFY(!controller.server().uaServer()->nodeById(QUaNodeId(id)));
    QVERIFY(logContains(controller.log(), QStringLiteral("cannot be instantiated as this node")));
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
}

QTEST_GUILESS_MAIN(TestTypeInstances)

#include "test_typeinstances.moc"
