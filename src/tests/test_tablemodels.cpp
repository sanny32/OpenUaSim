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
#include "subtypesmodel.h"
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

}

class TestTableModels : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void itemTableModelStoresItems();
    void tableModelsHaveNoChildren();
    void variablesModel();
    void variablesModelQuality_data();
    void variablesModelQuality();
    void variablesModelPassesTester();
    void subtypesModel();
    void referencesModel();
    void namespacesModel();
    void endpointsModel();
    void certificatesModel();
    void certificateStatus_data();
    void certificateStatus();
    void sessionsModel();
    void logModel();
    void logLevels_data();
    void logLevels();
    void logCategories();
};

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

void TestTableModels::tableModelsHaveNoChildren()
{
    VariablesModel model;
    model.setItems({ variable(QStringLiteral("a.Speed"), 1.0) });
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

    SubtypesModel model;
    model.setItems({ motor });
    QCOMPARE(model.columnCount(), int(SubtypesModel::ColCount));
    QCOMPARE(header(model, SubtypesModel::ColChildren), QStringLiteral("Children"));
    QCOMPARE(model.index(0, SubtypesModel::ColName).data().toString(), QStringLiteral("MotorType"));
    QCOMPARE(model.index(0, SubtypesModel::ColName).data(ItemRoles::IconName).toString(),
             QStringLiteral("folder"));
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
