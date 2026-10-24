// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

#include <QBuffer>
#include <QSignalSpy>
#include <QTest>
#include <QUaServer>

#include "project/projectnodes.h"
#include "server/addressspace.h"
#include "server/servercontroller.h"
#include "server/typecatalog.h"
#include "simulation/valuecast.h"
#include "testport.h"
#include "testproject.h"

namespace {

///
/// \brief Describes a project variable with an explicit variable type.
///
NodeDefinition variable(const QString &typeId, const QString &dataType, int length = 0)
{
    NodeDefinition node;
    node.kind = NodeDefinition::Kind::Variable;
    node.browseName = QStringLiteral("Value");
    node.nodeId = QStringLiteral("ns=1;s=Value");
    node.typeDefinition = typeId;
    node.dataType = dataType;
    node.arrayLength = length;
    node.initialValue = ValueCast::defaultValue(dataType);
    return node;
}

}

class TestVariableTypeRules : public QObject
{
    Q_OBJECT

private slots:
    void validatesShapes_data();
    void validatesShapes();
    void readsStandardConstraints();
    void readsImportedConstraints();
    void rejectsCreationAndEditingAtomically();
    void rejectsIncompatibleLoadedNodes();
    void supportedStandardTypesReceiveGoodValues();
};

///
/// \brief All supported OPC UA rank constraints are checked with fixed and unrestricted lengths.
///
void TestVariableTypeRules::validatesShapes_data()
{
    QTest::addColumn<int>("rank");
    QTest::addColumn<quint32>("dimension");
    QTest::addColumn<QString>("dataType");
    QTest::addColumn<int>("length");
    QTest::addColumn<QString>("error");
    QTest::newRow("any scalar") << -2 << 0u << "Double" << 0 << "";
    QTest::newRow("any vector") << -2 << 0u << "Double" << 4 << "";
    QTest::newRow("scalar") << -1 << 0u << "Double" << 0 << "";
    QTest::newRow("scalar rejects vector") << -1 << 0u << "Double" << 2 << "scalar";
    QTest::newRow("scalar or vector scalar") << -3 << 0u << "Double" << 0 << "";
    QTest::newRow("scalar or vector array") << -3 << 0u << "Double" << 2 << "";
    QTest::newRow("one or more scalar") << 0 << 0u << "Double" << 0 << "one-dimensional";
    QTest::newRow("one or more vector") << 0 << 0u << "Double" << 2 << "";
    QTest::newRow("vector scalar") << 1 << 0u << "Double" << 0 << "one-dimensional";
    QTest::newRow("vector") << 1 << 0u << "Double" << 2 << "";
    QTest::newRow("fixed vector") << 1 << 3u << "Double" << 3 << "";
    QTest::newRow("fixed vector mismatch") << 1 << 3u << "Double" << 2 << "3 elements";
    QTest::newRow("fixed vector too large") << 1 << 1001u << "Double" << 1000 << "1001 elements";
    QTest::newRow("matrix scalar") << 2 << 0u << "Double" << 0 << "multidimensional";
    QTest::newRow("matrix vector") << 2 << 0u << "Double" << 3 << "multidimensional";
    QTest::newRow("cube") << 3 << 0u << "Double" << 3 << "multidimensional";
    QTest::newRow("wrong data type") << -2 << 0u << "String" << 0 << "does not accept String";
    QTest::newRow("negative length") << -2 << 0u << "Double" << -1 << "between 0 and";
    QTest::newRow("too long") << -2 << 0u << "Double" << 1001 << "between 0 and";
    QTest::newRow("maximum length") << -2 << 0u << "Double" << 1000 << "";
}

///
/// \brief Shape validation has no dependency on a running server or a particular standard type name.
///
void TestVariableTypeRules::validatesShapes()
{
    QFETCH(int, rank);
    QFETCH(quint32, dimension);
    QFETCH(QString, dataType);
    QFETCH(int, length);
    QFETCH(QString, error);
    TypeInfo type;
    type.browseName = QStringLiteral("CustomType");
    type.dataTypeId = QStringLiteral("i=11");
    type.valueDataTypes = { QStringLiteral("Double") };
    type.valueRank = rank;
    if (dimension > 0)
        type.arrayDimensions = { dimension };
    const QString problem = TypeCatalog::variableProblem(type, dataType, length);
    if (error.isEmpty())
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    else
        QVERIFY2(problem.contains(error), qPrintable(problem));
    type.isAbstract = true;
    QVERIFY(TypeCatalog::variableProblem(type, dataType, length).contains(QStringLiteral("abstract")));
    type.isAbstract = false;
    type.valueDataTypes.clear();
    QVERIFY(!TypeCatalog::variableProblem(type, dataType, length).isEmpty());
    type.dataTypeId.clear();
    QVERIFY(TypeCatalog::variableProblem(type, dataType, length).contains(QStringLiteral("Cannot read")));
}

///
/// \brief Standard variable families restrict values through their data type hierarchy and ranks.
///
void TestVariableTypeRules::readsStandardConstraints()
{
    QUaServer server;
    const auto types = TypeCatalog::types(server);
    const TypeInfo analog = types.value(QStringLiteral("i=2368"));
    QVERIFY(analog.valueDataTypes.contains(QStringLiteral("Double")));
    QVERIFY(!analog.valueDataTypes.contains(QStringLiteral("String")));
    const TypeInfo discrete = types.value(QStringLiteral("i=2373"));
    QCOMPARE(discrete.valueDataTypes, QStringList { QStringLiteral("Boolean") });
    const TypeInfo image = types.value(QStringLiteral("i=12047"));
    QCOMPARE(image.valueRank, 2);
    QVERIFY(TypeCatalog::variableProblem(image, QStringLiteral("Double"), 0).contains(QStringLiteral("multidimensional")));
    const TypeInfo xy = types.value(QStringLiteral("i=12038"));
    QVERIFY(xy.valueDataTypes.isEmpty());
    QVERIFY(!TypeCatalog::variableProblem(xy, QStringLiteral("Double"), 1).isEmpty());
}

///
/// \brief Imported subtypes retain fixed array lengths and data type constraints.
///
void TestVariableTypeRules::readsImportedConstraints()
{
    QUaServer server;
    QByteArray xml = R"(<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
<NamespaceUris><Uri>urn:test:constraints</Uri></NamespaceUris>
<UAVariableType NodeId="ns=1;i=5000" BrowseName="1:Fixed" DataType="i=5" ValueRank="1" ArrayDimensions="3">
<References><Reference ReferenceType="i=45" IsForward="false">i=63</Reference></References>
</UAVariableType>
<UAVariableType NodeId="ns=1;i=5001" BrowseName="1:Derived" DataType="i=5" ValueRank="1" ArrayDimensions="3">
<References><Reference ReferenceType="i=45" IsForward="false">ns=1;i=5000</Reference></References>
</UAVariableType></UANodeSet>)";
    QBuffer buffer(&xml);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const auto loaded = server.loadNodeSet(&buffer);
    QVERIFY2(loaded.isOk(), qPrintable(loaded.errorString));
    QVERIFY2(loaded.warnings.isEmpty(), qPrintable(loaded.warnings.join(QLatin1Char('\n'))));
    const QString portable = QStringLiteral("nsu=urn:test:constraints;i=5001");
    QVERIFY(TypeCatalog::variableProblem(server, variable(portable, QStringLiteral("UInt16"), 3)).isEmpty());
    QVERIFY(TypeCatalog::variableProblem(server, variable(portable, QStringLiteral("UInt16"), 2))
                .contains(QStringLiteral("3 elements")));
    QVERIFY(TypeCatalog::variableProblem(server, variable(portable, QStringLiteral("Double"), 3))
                .contains(QStringLiteral("does not accept Double")));
    QVERIFY(!TypeCatalog::variableProblem(server, variable(QStringLiteral("nsu=urn:missing;i=1"), "Double")).isEmpty());
    QVERIFY(!TypeCatalog::variableProblem(server, variable(QStringLiteral("i=58"), "Double")).isEmpty());
}

///
/// \brief Refused creations and edits leave the project and existing live values unchanged.
///
void TestVariableTypeRules::rejectsCreationAndEditingAtomically()
{
    ServerController controller;
    ProjectConfig project = TestProject::create();
    project.endpoints.first().port = TestPort::freePort();
    controller.setProject(project);
    QSignalSpy edited(&controller, &ServerController::projectEdited);
    QString error;
    QVERIFY(controller.addNode({}, variable("i=12047", "Double"), &error).isEmpty());
    QVERIFY(error.contains(QStringLiteral("multidimensional")));
    QVERIFY(!ProjectNodes::find(controller.project().nodes, QStringLiteral("ns=1;s=Value")));
    QCOMPARE(edited.count(), 0);
    NodeDefinition folder;
    folder.browseName = QStringLiteral("InvalidParent");
    folder.children = { variable("i=2373", "Double") };
    QVERIFY(controller.addNode({}, folder, &error).isEmpty());
    QCOMPARE(edited.count(), 0);

    const QString id = controller.addNode({}, variable("i=2373", "Boolean"), &error);
    QVERIFY2(!id.isEmpty(), qPrintable(error));
    QCOMPARE(controller.addressSpace().quality(id), VariableInfo::Quality::Good);
    edited.clear();
    VariableInfo change;
    change.nodeId = id;
    change.displayName = QStringLiteral("Value");
    change.access = QStringLiteral("Read/Write");
    change.dataType = QStringLiteral("Double");
    QVERIFY(!controller.updateVariable(change, &error));
    QVERIFY(error.contains(QStringLiteral("does not accept Double")));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, id)->dataType, QStringLiteral("Boolean"));
    QCOMPARE(controller.addressSpace().quality(id), VariableInfo::Quality::Good);
    QCOMPARE(edited.count(), 0);
    change.dataType = QStringLiteral("Boolean");
    change.arrayLength = 2;
    QVERIFY2(controller.updateVariable(change, &error), qPrintable(error));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, id)->arrayLength, 2);
    QCOMPARE(controller.addressSpace().quality(id), VariableInfo::Quality::Good);
    NodeDefinition vector = variable("i=12029", "Double", 2);
    vector.nodeId = QStringLiteral("ns=1;s=Vector");
    vector.browseName = QStringLiteral("Vector");
    const QString vectorId = controller.addNode({}, vector, &error);
    QVERIFY2(!vectorId.isEmpty(), qPrintable(error));
    edited.clear();
    change.nodeId = vectorId;
    change.displayName = vector.browseName;
    change.dataType = vector.dataType;
    change.arrayLength = 0;
    QVERIFY(!controller.updateVariable(change, &error));
    QVERIFY(error.contains(QStringLiteral("one-dimensional")));
    QCOMPARE(ProjectNodes::find(controller.project().nodes, vectorId)->arrayLength, 2);
    QCOMPARE(edited.count(), 0);
}

///
/// \brief Invalid definitions loaded from older projects never receive incompatible values.
///
void TestVariableTypeRules::rejectsIncompatibleLoadedNodes()
{
    QUaServer server;
    AddressSpace space(&server);
    const auto image = variable("i=12047", "Double");
    const QStringList problems = space.build({ image });
    QCOMPARE(problems.size(), 1);
    QVERIFY(problems.first().contains(QStringLiteral("multidimensional")));
    QVERIFY(!space.hasVariable(image.nodeId));
    QVERIFY(!server.isNodeIdUsed(QUaNodeId(image.nodeId)));
}

///
/// \brief Every supported standard variable type accepts its configured initial value with Good quality.
///
void TestVariableTypeRules::supportedStandardTypesReceiveGoodValues()
{
    QUaServer server;
    AddressSpace space(&server);
    const auto types = TypeCatalog::instanceTypes(TypeCatalog::types(server), NodeDefinition::Kind::Variable);
    int checked = 0;
    int rejected = 0;
    for (const TypeInfo &type : types) {
        const QString dataType = type.valueDataTypes.value(0, QStringLiteral("Double"));
        const int length = type.valueRank >= 0 ? qMax(1, int(type.arrayDimensions.value(0))) : 0;
        const NodeDefinition node = variable(type.nodeId, dataType, length);
        if (!TypeCatalog::variableProblem(type, dataType, length).isEmpty()) {
            ++rejected;
            continue;
        }
        const QStringList problems = space.build({ node });
        QVERIFY2(problems.isEmpty(), qPrintable(type.browseName + QLatin1Char(':') + problems.join(QLatin1Char('\n'))));
        QVERIFY2(space.hasVariable(node.nodeId), qPrintable(type.browseName));
        QCOMPARE(space.quality(node.nodeId), VariableInfo::Quality::Good);
        QVERIFY2(space.value(node.nodeId).isValid(), qPrintable(type.browseName));
        ++checked;
    }
    QVERIFY(checked > 5);
    QVERIFY(rejected > 1);
}

QTEST_GUILESS_MAIN(TestVariableTypeRules)

#include "test_variabletyperules.moc"
