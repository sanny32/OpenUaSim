// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file mockdata.cpp
/// \brief Implements the sample server state the UI tests show.
///

#include <algorithm>

#include "mockdata.h"

namespace MockData {

namespace {

const QString kLocalEndpoint = QStringLiteral("opc.tcp://localhost:4840");

///
/// \brief Builds a timestamp on the sample day.
/// \param time Time of day as "HH:mm:ss.zzz".
/// \return Local timestamp on 2024-05-23.
///
QDateTime at(const QString &time)
{
    return QDateTime(QDate(2024, 5, 23), QTime::fromString(time, QStringLiteral("HH:mm:ss.zzz")));
}

///
/// \brief Builds a simulated variable of a device.
/// \param device Device browse name, used to derive the node id.
/// \param name Variable browse name.
/// \param icon Icon resource name.
/// \param value Current value.
/// \param simulation Simulation driving the value.
/// \return Read/write variable of Good quality.
///
VariableInfo variable(const QString &device, const QString &name, const QString &icon,
                      const QVariant &value, const SimulationSettings &simulation = {})
{
    VariableInfo info;
    info.nodeId = QStringLiteral("ns=2;s=%1.%2").arg(device, name);
    info.displayName = name;
    info.icon = icon;
    info.value = value;
    info.dataType = value.typeId() == QMetaType::Bool ? QStringLiteral("Boolean")
                                                      : QStringLiteral("Double");
    info.access = QStringLiteral("Read/Write");
    info.simulation = simulation;
    return info;
}

///
/// \brief Builds the simulation settings of a variable.
/// \param function Waveform.
/// \param intervalMs Update interval in milliseconds.
/// \param amplitude Peak deviation.
/// \param offset Centre value.
/// \return Simulation settings with the default period.
///
SimulationSettings simulation(SimulationFunction function, int intervalMs, double amplitude,
                              double offset)
{
    SimulationSettings settings;
    settings.function = function;
    settings.intervalMs = intervalMs;
    settings.amplitude = amplitude;
    settings.offset = offset;
    return settings;
}

///
/// \brief Builds the variables of every sample device.
/// \return Variables keyed by device browse name, in display order.
///
QList<QPair<QString, QList<VariableInfo>>> devices()
{
    const QString motor = QStringLiteral("Motor_01");
    const QString conveyor = QStringLiteral("Conveyor_01");
    const QString pump = QStringLiteral("Pump_01");
    const QString tank = QStringLiteral("Tank_01");
    const QString signal = QStringLiteral("signal");
    const QString thermometer = QStringLiteral("thermometer");
    const QString running = QStringLiteral("status-good");

    VariableInfo pumpPressure = variable(pump, QStringLiteral("Pressure"), QStringLiteral("gauge"),
                                         4.1, simulation(SimulationFunction::Random, 500, 0.5, 4.0));
    pumpPressure.quality = VariableInfo::Quality::Uncertain;

    return {
        { motor, {
            variable(motor, QStringLiteral("Speed"), signal, 1487.6,
                     simulation(SimulationFunction::Sine, 100, 500.0, 1000.0)),
            variable(motor, QStringLiteral("Temperature"), thermometer, 46.2,
                     simulation(SimulationFunction::Ramp, 1000, 10.0, 40.0)),
            variable(motor, QStringLiteral("Running"), running, true),
        } },
        { conveyor, {
            variable(conveyor, QStringLiteral("Speed"), signal, 1.2,
                     simulation(SimulationFunction::Square, 2000, 0.4, 1.0)),
            variable(conveyor, QStringLiteral("Load"), QStringLiteral("gauge"), 62.5,
                     simulation(SimulationFunction::Random, 1000, 20.0, 60.0)),
            variable(conveyor, QStringLiteral("Running"), running, true),
        } },
        { pump, {
            variable(pump, QStringLiteral("Flow"), signal, 12.8,
                     simulation(SimulationFunction::Sine, 250, 3.0, 12.0)),
            pumpPressure,
            variable(pump, QStringLiteral("Running"), running, true),
        } },
        { tank, {
            variable(tank, QStringLiteral("Level"), QStringLiteral("gauge"), 73.4,
                     simulation(SimulationFunction::Triangle, 1000, 25.0, 50.0)),
            variable(tank, QStringLiteral("Temperature"), thermometer, 21.7,
                     simulation(SimulationFunction::Random, 5000, 1.5, 21.0)),
            variable(tank, QStringLiteral("InletValve"), QStringLiteral("status-inactive"), false),
        } },
    };
}

///
/// \brief Builds a tree node.
/// \param text Node text.
/// \param icon Icon resource name.
/// \param kind Node kind.
/// \param id Node identifier.
/// \return Collapsed, unselected node without children.
///
TreeNode node(const QString &text, const QString &icon, TreeNode::Kind kind,
              const QString &id = QString())
{
    TreeNode result;
    result.text = text;
    result.icon = icon;
    result.kind = kind;
    result.id = id;
    return result;
}

///
/// \brief Builds the address space and the variables of every object.
/// \param snapshot Snapshot receiving the address space tree and the variables.
///
void fillAddressSpace(ServerSnapshot &snapshot)
{
    const QString folder = QStringLiteral("folder");

    TreeNode line = node(QStringLiteral("FactoryLine_01"), folder, TreeNode::Kind::Folder,
                         QStringLiteral("ns=2;s=FactoryLine_01"));
    line.expanded = true;

    for (const auto &[device, variables] : devices()) {
        const QString objectId = QStringLiteral("ns=2;s=%1").arg(device);
        TreeNode object = node(device, folder, TreeNode::Kind::Object, objectId);
        for (const VariableInfo &info : variables) {
            TreeNode child = node(info.displayName, info.icon, TreeNode::Kind::Variable, info.nodeId);
            object.children.append(child);
        }
        snapshot.variables.insert(objectId, variables);
        line.children.append(object);
    }

    TreeNode &motor = line.children.first();
    motor.expanded = true;
    motor.children.first().current = true;

    TreeNode objects = node(QStringLiteral("Objects"), folder, TreeNode::Kind::Folder,
                            QStringLiteral("i=85"));
    objects.expanded = true;
    objects.children.append(line);

    TreeNode types = node(QStringLiteral("Types"), folder, TreeNode::Kind::Folder,
                          QStringLiteral("i=86"));
    types.children.append(node(QStringLiteral("ObjectTypes"), folder, TreeNode::Kind::Folder,
                               QStringLiteral("i=88")));
    types.children.append(node(QStringLiteral("VariableTypes"), folder, TreeNode::Kind::Folder,
                               QStringLiteral("i=89")));

    TreeNode views = node(QStringLiteral("Views"), folder, TreeNode::Kind::Folder,
                          QStringLiteral("i=87"));

    snapshot.addressSpace = { objects, types, views };
}

///
/// \brief Builds a type.
/// \param name Browse name.
/// \param nodeId Node id.
/// \param baseTypeId Supertype node id.
/// \param children Number of declared children.
/// \param description Description text.
/// \return ObjectType description with sample reference counts.
///
TypeInfo type(const QString &name, const QString &nodeId, const QString &baseTypeId,
              int children, const QString &description)
{
    TypeInfo info;
    info.browseName = name;
    info.nodeId = nodeId;
    info.baseTypeId = baseTypeId;
    info.category = QStringLiteral("ObjectType");
    info.children = children;
    info.description = description;
    info.references = {
        { QStringLiteral("HasSubtype"), 0 },
        { QStringLiteral("HasComponent"), children },
        { QStringLiteral("Organizes"), 0 },
        { QStringLiteral("HasProperty"), 2 },
    };
    return info;
}

///
/// \brief Builds the type system and its navigation tree.
/// \param snapshot Snapshot receiving the types and the type tree.
///
void fillTypes(ServerSnapshot &snapshot)
{
    const QString baseObject = QStringLiteral("i=58");
    const QString baseVariable = QStringLiteral("i=62");
    const QString device = QStringLiteral("ns=2;i=1000");

    QList<TypeInfo> types = {
        type(QStringLiteral("BaseObjectType"), baseObject, QString(), 0,
             QStringLiteral("Base type for all objects.")),
        type(QStringLiteral("FolderType"), QStringLiteral("i=61"), baseObject, 0,
             QStringLiteral("Type for folder objects that organize other nodes.")),
        type(QStringLiteral("DeviceType"), device, baseObject, 4,
             QStringLiteral("Base type for all devices in the application model.")),
        type(QStringLiteral("MotorType"), QStringLiteral("ns=2;i=1001"), device, 8,
             QStringLiteral("Electric motor with speed and temperature.")),
        type(QStringLiteral("PumpType"), QStringLiteral("ns=2;i=1002"), device, 6,
             QStringLiteral("Centrifugal pump with flow and pressure.")),
        type(QStringLiteral("SensorType"), QStringLiteral("ns=2;i=1003"), device, 5,
             QStringLiteral("Generic analog sensor.")),
        type(QStringLiteral("BaseVariableType"), baseVariable, QString(), 0,
             QStringLiteral("Base type for all variables.")),
        type(QStringLiteral("BaseDataVariableType"), QStringLiteral("i=63"), baseVariable, 0,
             QStringLiteral("Type for variables that hold data.")),
        type(QStringLiteral("PropertyType"), QStringLiteral("i=68"), baseVariable, 0,
             QStringLiteral("Type for variables that describe a node.")),
    };
    types[0].isAbstract = true;
    types[6].isAbstract = true;
    for (TypeInfo &info : types) {
        info.category = info.baseTypeId == baseVariable || info.nodeId == baseVariable
                            ? QStringLiteral("VariableType")
                            : QStringLiteral("ObjectType");
    }

    for (const TypeInfo &info : types)
        snapshot.types.insert(info.nodeId, info);
    for (TypeInfo &info : snapshot.types) {
        const auto subtypes = std::count_if(types.cbegin(), types.cend(), [&info](const TypeInfo &other) {
            return other.baseTypeId == info.nodeId;
        });
        info.references.first().count = static_cast<int>(subtypes);
    }
}

///
/// \brief Builds the namespace array.
/// \return Standard and application namespaces.
///
QList<NamespaceInfo> namespaces()
{
    return {
        { 0, QStringLiteral("http://opcfoundation.org/UA/"), QStringLiteral("1.05"),
          QDate(2017, 7, 10),
          QStringLiteral("OPC UA standard namespace containing base definitions for nodes, "
                         "types and services.") },
        { 1, QStringLiteral("http://opcfoundation.org/UA/DI/"), QStringLiteral("1.00"),
          QDate(2019, 1, 22), QStringLiteral("OPC UA for Devices companion specification.") },
        { 2, QStringLiteral("urn:openuasim:factory"), QStringLiteral("1.0.0"),
          QDate(2024, 5, 20), QStringLiteral("Sample factory line objects.") },
        { 3, QStringLiteral("urn:openuasim:machinery"), QStringLiteral("1.0.0"),
          QDate(2024, 5, 20), QStringLiteral("Machinery building blocks.") },
        { 4, QStringLiteral("urn:openuasim:types"), QStringLiteral("1.0.0"),
          QDate(2024, 5, 20), QStringLiteral("Application type definitions.") },
        { 5, QStringLiteral("urn:openuasim:models"), QStringLiteral("1.0.0"),
          QDate(2024, 5, 21), QStringLiteral("Imported information models.") },
    };
}

///
/// \brief Builds an endpoint.
/// \param hostname Host name.
/// \param path URL path.
/// \param policy Security policy.
/// \param mode Security mode.
/// \return Enabled opc.tcp endpoint on port 4840 accepting anonymous users.
///
EndpointInfo endpoint(const QString &hostname, const QString &path, const QString &policy, const QString &mode)
{
    EndpointInfo info;
    info.protocol = QStringLiteral("opc.tcp");
    info.hostname = hostname;
    info.port = 4840;
    info.path = path;
    info.securityPolicy = policy;
    info.securityMode = mode;
    return info;
}

///
/// \brief Builds the configured endpoints.
/// \return Local, LAN, wildcard and discovery endpoints.
///
QList<EndpointInfo> endpoints()
{
    return {
        endpoint(QStringLiteral("localhost"), QString(), QStringLiteral("Basic256Sha256"),
                 QStringLiteral("Sign & Encrypt")),
        endpoint(QStringLiteral("192.168.1.100"), QString(), QStringLiteral("Basic256Sha256"),
                 QStringLiteral("Sign & Encrypt")),
        endpoint(QStringLiteral("0.0.0.0"), QString(), QStringLiteral("Basic256"), QStringLiteral("Sign")),
        endpoint(QStringLiteral("localhost"), QStringLiteral("/discovery"), QStringLiteral("None"),
                 QStringLiteral("None")),
    };
}

///
/// \brief Builds a certificate.
/// \param category Store folder.
/// \param commonName Subject common name.
/// \param serial Serial number.
/// \param from Start of validity.
/// \param to End of validity.
/// \param fingerprint SHA-1 fingerprint.
/// \param file File name below the folder.
/// \return Certificate issued by the server CA.
///
CertificateInfo certificate(CertificateInfo::Category category, const QString &commonName,
                            const QString &serial, const QDateTime &from, const QDateTime &to,
                            const QString &fingerprint, const QString &file)
{
    static const char *const folders[] = { "own", "trusted", "rejected", "issuers" };

    CertificateInfo info;
    info.category = category;
    info.commonName = commonName;
    info.version = QStringLiteral("V3");
    info.signatureAlgorithm = QStringLiteral("sha256WithRSAEncryption");
    info.signatureHashAlgorithm = QStringLiteral("SHA256");
    info.publicKey = QStringLiteral("RSA (2048 bits)");
    info.publicKeyParameters = QStringLiteral("05 00");
    info.basicConstraints = QStringLiteral("CA:FALSE");
    info.extendedKeyUsage = QStringLiteral("TLS Web Server Authentication");
    info.subjectKeyIdentifier = fingerprint;
    info.authorityKeyIdentifier = fingerprint;
    info.subjectAlternativeNames = QStringLiteral("URI:urn:openuasim:server, DNS:localhost");
    info.subject = QStringLiteral("CN=%1,\nO=Example Corp,\nC=US").arg(commonName);
    info.issuer = QStringLiteral("CN=OpenUaSim,\nO=Example Corp,\nC=US");
    info.serialNumber = serial;
    info.validFrom = from;
    info.validTo = to;
    info.fingerprint = fingerprint;
    info.keyUsage = QStringLiteral("Digital Signature, Key Encipherment");
    info.filePath = QStringLiteral("C:/ProgramData/OpenUaSim/pki/%1/certs/%2")
                        .arg(QLatin1String(folders[static_cast<int>(category)]), file);
    return info;
}

///
/// \brief Builds the certificates of every store folder.
/// \return Own, trusted, rejected and issuer certificates.
///
QList<CertificateInfo> certificates()
{
    using Category = CertificateInfo::Category;
    const auto date = [](int year, int month, int day, int hour, int minute) {
        return QDateTime(QDate(year, month, day), QTime(hour, minute));
    };

    return {
        certificate(Category::Application, QStringLiteral("OpenUaSim"),
                    QStringLiteral("0A 1B 2C 3D 4E 5F 60 71"), date(2024, 1, 1, 0, 0),
                    date(2029, 1, 1, 0, 0), QStringLiteral("5E:21:9A:0C:44:7D:18:B2:E9:3F"),
                    QStringLiteral("openuasim.der")),
        certificate(Category::Trusted, QStringLiteral("OPC UA Client A"),
                    QStringLiteral("01 23 45 67 89 AB CD EF"), date(2024, 4, 10, 10, 0),
                    date(2026, 4, 10, 10, 0), QStringLiteral("3A:5F:7C:9D:12:34:56:78:9A:BC"),
                    QStringLiteral("clientA.der")),
        certificate(Category::Trusted, QStringLiteral("EngineeringTool"),
                    QStringLiteral("11 22 33 44 55 66 77 88"), date(2024, 1, 15, 8, 30),
                    date(2027, 1, 15, 8, 30), QStringLiteral("8B:21:4D:6E:9F:10:22:33:44:55"),
                    QStringLiteral("engineeringtool.der")),
        certificate(Category::Trusted, QStringLiteral("SCADA_System"),
                    QStringLiteral("A1 B2 C3 D4 E5 F6 07 18"), date(2023, 11, 5, 14, 20),
                    date(2026, 11, 5, 14, 20), QStringLiteral("C1:D4:5E:6F:77:88:99:AA:BB:CC"),
                    QStringLiteral("scada.der")),
        certificate(Category::Rejected, QStringLiteral("UnknownClient"),
                    QStringLiteral("FF EE DD CC BB AA 99 88"), date(2024, 5, 1, 9, 0),
                    date(2025, 5, 1, 9, 0), QStringLiteral("9F:8E:7D:6C:5B:4A:39:28:17:06"),
                    QStringLiteral("unknown.der")),
        certificate(Category::Issuer, QStringLiteral("OpenUaSim CA"),
                    QStringLiteral("00 00 00 00 00 00 00 01"), date(2024, 1, 1, 0, 0),
                    date(2034, 1, 1, 0, 0), QStringLiteral("11:22:33:44:55:66:77:88:99:00"),
                    QStringLiteral("ca.der")),
        certificate(Category::Issuer, QStringLiteral("Example Corp CA"),
                    QStringLiteral("00 00 00 00 00 00 00 02"), date(2023, 1, 1, 0, 0),
                    date(2033, 1, 1, 0, 0), QStringLiteral("AA:BB:CC:DD:EE:FF:00:11:22:33"),
                    QStringLiteral("examplecorp-ca.der")),
    };
}

///
/// \brief Builds a session.
/// \param id Numeric part of the session node id.
/// \param name Session name.
/// \param client Client product.
/// \param user User name.
/// \param subscriptions Subscription count; non-zero marks the session active.
/// \param created Creation time.
/// \param lastActivity Last request time.
/// \return Session over the local endpoint without security.
///
SessionInfo session(int id, const QString &name, const QString &client, const QString &user,
                    int subscriptions, const QDateTime &created, const QDateTime &lastActivity)
{
    SessionInfo info;
    info.sessionId = QStringLiteral("ns=1;i=%1").arg(id);
    info.name = name;
    info.client = client;
    info.user = user;
    info.active = subscriptions > 0;
    info.subscriptions = subscriptions;
    info.created = created;
    info.lastActivity = lastActivity;
    info.applicationUri = QStringLiteral("urn:%1").arg(QString(name).remove(QLatin1Char(' ')));
    info.endpointUrl = kLocalEndpoint;
    info.securityMode = QStringLiteral("None");
    info.userIdentity = user == QLatin1String("anonymous") ? QStringLiteral("Anonymous")
                                                           : QStringLiteral("Username");
    return info;
}

///
/// \brief Builds the active and closed sessions.
/// \return One active and four closed sessions.
///
QList<SessionInfo> sessions()
{
    SessionInfo uaExpert = session(12345, QStringLiteral("UaExpert"), QStringLiteral("UaExpert/1.6.4"),
                                   QStringLiteral("anonymous"), 3, at(QStringLiteral("14:30:12.116")),
                                   at(QStringLiteral("14:31:52.004")));
    uaExpert.applicationUri = QStringLiteral("urn:unifiedautomation:UaExpert");

    return {
        uaExpert,
        session(12344, QStringLiteral("QuickClient"), QStringLiteral("QuickOPC/5.2.1"),
                QStringLiteral("user1"), 0, at(QStringLiteral("14:20:01.000")),
                at(QStringLiteral("14:28:11.532"))),
        session(12343, QStringLiteral("OpcUaClient"), QStringLiteral("OPC Foundation/1.4.0"),
                QStringLiteral("anonymous"), 0, at(QStringLiteral("14:18:44.000")),
                at(QStringLiteral("14:25:04.921"))),
        session(12342, QStringLiteral("Prosys Client"), QStringLiteral("Prosys OPC UA Client/5.1"),
                QStringLiteral("operator"), 0, at(QStringLiteral("14:10:02.000")),
                at(QStringLiteral("14:21:37.648"))),
        session(12341, QStringLiteral("UaExpert (old)"), QStringLiteral("UaExpert/1.6.3"),
                QStringLiteral("anonymous"), 0, at(QStringLiteral("14:02:15.000")),
                at(QStringLiteral("14:17:09.000"))),
    };
}

///
/// \brief Builds a log entry.
/// \param time Time of day as "HH:mm:ss.zzz".
/// \param level Severity.
/// \param category Producing subsystem.
/// \param message Message text.
/// \return Log entry produced by the simulator.
///
LogEntry entry(const QString &time, LogEntry::Level level, LogEntry::Category category,
               const QString &message)
{
    return { at(time), level, category, message, QStringLiteral("OpenUaSim") };
}

///
/// \brief Builds the server log.
/// \return Start-up, session, security and data change entries.
///
QList<LogEntry> log()
{
    using Level = LogEntry::Level;
    using Category = LogEntry::Category;
    return {
        entry(QStringLiteral("14:31:20.004"), Level::Info, Category::Server,
              QStringLiteral("Server started. Endpoint: %1").arg(kLocalEndpoint)),
        entry(QStringLiteral("14:31:25.123"), Level::Info, Category::Session,
              QStringLiteral("Client connected: opc.tcp://127.0.0.1:49732")),
        entry(QStringLiteral("14:31:25.456"), Level::Info, Category::Session,
              QStringLiteral("Session created (SessionId: ns=1;i=12345)")),
        entry(QStringLiteral("14:31:26.018"), Level::Info, Category::Security,
              QStringLiteral("Secure channel opened (SecurityPolicy: Basic256Sha256)")),
        entry(QStringLiteral("14:31:26.521"), Level::Info, Category::Session,
              QStringLiteral("User 'Anonymous' authenticated")),
        entry(QStringLiteral("14:31:27.104"), Level::Info, Category::DataChanges,
              QStringLiteral("Monitored item created: ns=2;s=Motor_01.Speed")),
        entry(QStringLiteral("14:31:28.710"), Level::Warning, Category::Server,
              QStringLiteral("Slow client detected (publishing interval: 1000 ms)")),
        entry(QStringLiteral("14:31:29.332"), Level::Info, Category::DataChanges,
              QStringLiteral("Value updated: ns=2;s=Motor_01.Speed = 1 487.6")),
        entry(QStringLiteral("14:31:31.405"), Level::Info, Category::DataChanges,
              QStringLiteral("Value updated: ns=2;s=Motor_01.Temperature = 46.2")),
        entry(QStringLiteral("14:31:35.872"), Level::Info, Category::Session,
              QStringLiteral("Client disconnected: opc.tcp://127.0.0.1:49732")),
        entry(QStringLiteral("14:31:35.873"), Level::Info, Category::Session,
              QStringLiteral("Session closed (SessionId: ns=1;i=12344)")),
        entry(QStringLiteral("14:31:40.112"), Level::Error, Category::Security,
              QStringLiteral("Invalid user token in request (BadUserAccessDenied)")),
        entry(QStringLiteral("14:31:42.998"), Level::Info, Category::Server,
              QStringLiteral("Listening for connections on %1").arg(kLocalEndpoint)),
    };
}

///
/// \brief Counts the nodes of a tree.
/// \param nodes Tree level to count.
/// \return Number of nodes including all descendants.
///
int countNodes(const QList<TreeNode> &nodes)
{
    int count = 0;
    for (const TreeNode &child : nodes)
        count += 1 + countNodes(child.children);
    return count;
}

}

///
/// \brief Builds a sample factory-line server state.
/// \return Snapshot with a running server, an address space of four devices, a small
///         type system, namespaces, endpoints, certificates, sessions and a log.
///
ServerSnapshot snapshot()
{
    ServerSnapshot snapshot;
    fillAddressSpace(snapshot);
    fillTypes(snapshot);
    snapshot.namespaces = namespaces();
    snapshot.endpoints = endpoints();
    snapshot.certificates = certificates();
    snapshot.sessions = sessions();
    snapshot.log = log();

    ServerStatus &status = snapshot.status;
    status.running = true;
    status.primaryEndpoint = kLocalEndpoint;
    status.applicationUri = QStringLiteral("urn:OpenUaSim:localhost:4840");
    status.startTime = at(QStringLiteral("14:31:20.000"));
    status.uptimeSeconds = 27 * 60 + 36;
    status.securityModes = { QStringLiteral("None"), QStringLiteral("Sign"),
                             QStringLiteral("Sign & Encrypt") };
    status.totalNodes = countNodes(snapshot.addressSpace);
    status.namespaces = static_cast<int>(snapshot.namespaces.size());
    status.clientsConnected = static_cast<int>(
        std::count_if(snapshot.sessions.cbegin(), snapshot.sessions.cend(),
                      [](const SessionInfo &info) { return info.active; }));
    return snapshot;
}

}
