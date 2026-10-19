// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file simserver.cpp
/// \brief Implements the OPC UA server the simulator runs.
///

#include <QTimer>

#include <QUaServer>

#include "listenersettings.h"
#include "logmapper.h"
#include "simserver.h"

namespace {

constexpr int kDiagnosticsRefreshMs = 2000;

///
/// \brief Returns the display name of a message security mode.
/// \param mode QUaServer mode.
/// \return "None", "Sign" or "Sign & Encrypt", the names endpoints use.
///
QString securityModeName(QUaMessageSecurityMode mode)
{
    switch (mode) {
    case QUaMessageSecurityMode::Sign:           return QStringLiteral("Sign");
    case QUaMessageSecurityMode::SignAndEncrypt: return QStringLiteral("Sign & Encrypt");
    default:                                     return QStringLiteral("None");
    }
}

///
/// \brief Describes a QUaServer session for the sessions page.
/// \param session Session reported by QUaServer.
/// \param endpointUrl URL the server listens on, used when the session does not report the one it asked for.
/// \return Active session description.
///
SessionInfo sessionInfo(const QUaSession *session, const QString &endpointUrl)
{
    SessionInfo info;
    info.sessionId = session->sessionId();
    info.name = session->applicationName();
    info.client = session->productUri().isEmpty() ? session->applicationName() : session->productUri();
    info.user = session->isAnonymous() ? QObject::tr("Anonymous") : session->userName();
    info.active = true;
    info.created = session->timestamp().isValid() ? session->timestamp().toLocalTime() : QDateTime::currentDateTime();
    info.lastActivity = info.created;
    info.applicationUri = session->applicationUri();
    info.endpointUrl = session->endpointUrl().isEmpty() ? endpointUrl : session->endpointUrl();
    info.securityMode = securityModeName(session->securityMode());
    info.userIdentity = session->isAnonymous() ? QStringLiteral("Anonymous") : QStringLiteral("UserName");
    return info;
}

#ifdef OUASIM_HAVE_ENCRYPTION
///
/// \brief Converts security policy names into QUaServer flags.
/// \param names Names from ListenerSettingsBuilder::securityPolicyNames().
/// \return Matching flags.
///
QUaSecurityPolicies securityPolicies(const QStringList &names)
{
    const QList<QPair<QString, QUaSecurityPolicy>> table = {
        { QStringLiteral("None"), QUaSecurityPolicy::None },
        { QStringLiteral("Basic128Rsa15"), QUaSecurityPolicy::Basic128Rsa15 },
        { QStringLiteral("Basic256"), QUaSecurityPolicy::Basic256 },
        { QStringLiteral("Basic256Sha256"), QUaSecurityPolicy::Basic256Sha256 },
        { QStringLiteral("Aes128_Sha256_RsaOaep"), QUaSecurityPolicy::Aes128Sha256RsaOaep },
        { QStringLiteral("Aes256_Sha256_RsaPss"), QUaSecurityPolicy::Aes256Sha256RsaPss },
    };
    QUaSecurityPolicies policies;
    for (const auto &[name, policy] : table) {
        if (names.contains(name))
            policies |= policy;
    }
    return policies;
}

///
/// \brief Converts message security mode names into QUaServer flags.
/// \param names Names from ListenerSettingsBuilder::securityModeNames().
/// \return Matching flags.
///
QUaMessageSecurityModes securityModes(const QStringList &names)
{
    QUaMessageSecurityModes modes;
    if (names.contains(QStringLiteral("None")))
        modes |= QUaMessageSecurityMode::None;
    if (names.contains(QStringLiteral("Sign")))
        modes |= QUaMessageSecurityMode::Sign;
    if (names.contains(QStringLiteral("Sign & Encrypt")))
        modes |= QUaMessageSecurityMode::SignAndEncrypt;
    return modes;
}
#endif

}

///
/// \brief Creates a stopped server.
/// \param parent Owning QObject.
///
SimServer::SimServer(QObject *parent)
    : QObject(parent)
    , _diagnosticsTimer(new QTimer(this))
{
    _diagnosticsTimer->setInterval(kDiagnosticsRefreshMs);
    connect(_diagnosticsTimer, &QTimer::timeout, this, &SimServer::refreshSessionDiagnostics);
    createServer();
}

///
/// \brief Stops the server and releases it.
///
SimServer::~SimServer()
{
    releaseServer();
}

///
/// \brief Replaces the server with an empty one, dropping every node and namespace loaded into it.
///        A running server is stopped first; the next start() serves the new one.
///
void SimServer::reset()
{
    stop();
    releaseServer();
    createServer();
}

///
/// \brief Creates the QUaServer and forwards its log, session and certificate events.
///
void SimServer::createServer()
{
    _server = new QUaServer(this);
    connect(_server, &QUaServer::logMessage, this, &SimServer::handleLog);
    connect(_server, &QUaServer::clientConnected, this, &SimServer::handleClientConnected);
    connect(_server, &QUaServer::clientDisconnected, this, &SimServer::handleClientDisconnected);
#ifdef OUASIM_HAVE_ENCRYPTION
    connect(_server, &QUaServer::clientCertificateRejected, this, &SimServer::handleCertificateRejected);
#endif
}

///
/// \brief Stops and deletes the QUaServer.
///
void SimServer::releaseServer()
{
    // Disconnect first so stopping does not call back into a half-destroyed object.
    _server->disconnect(this);
    _server->stop();
    delete _server;
    _server = nullptr;
}

///
/// \brief Applies the project's server settings and endpoints and starts listening.
/// \param project Project to serve; its nodes are built separately.
/// \return True when the server runs; otherwise startFailed() tells why.
///
bool SimServer::start(const ProjectConfig &project)
{
    if (_server->isRunning())
        return true;

    const ListenerSettings listener = ListenerSettingsBuilder::build(project.endpoints);
    if (!listener.ok())
        return fail(listener.error);
    if (!configure(project, listener))
        return false;

    if (!_server->start())
        return fail(tr("The server could not listen on port %1; it may be in use by another program")
                        .arg(listener.port));

    _sessions.clear();
    _startTime = QDateTime::currentDateTime();
    log(LogEntry::Level::Info, LogEntry::Category::Server, tr("Server started on %1").arg(_endpointUrl));
    emit sessionsChanged();
    _diagnosticsTimer->start();
    emit runningChanged(true);
    return true;
}

///
/// \brief Sets the certificates applied on the next start; ignored in builds without encryption.
/// \param material Server certificate and key with the trusted and issuer certificates.
///
void SimServer::setPkiMaterial(const PkiMaterial &material)
{
    _pki = material;
}

///
/// \brief Stops listening and closes every session; does nothing when stopped.
///
void SimServer::stop()
{
    if (!_server->isRunning())
        return;
    _diagnosticsTimer->stop();
    _server->stop();
    _sessions.closeAll(QDateTime::currentDateTime());
    log(LogEntry::Level::Info, LogEntry::Category::Server, tr("Server stopped"));
    emit sessionsChanged();
    emit runningChanged(false);
}

///
/// \brief Tells whether the server is listening.
/// \return True between a successful start() and stop().
///
bool SimServer::isRunning() const
{
    return _server->isRunning();
}

///
/// \brief Returns when the server last started.
/// \return Start time, or an invalid time when it never ran.
///
QDateTime SimServer::startTime() const
{
    return _startTime;
}

///
/// \brief Returns the URL clients connect to.
/// \return URL such as "opc.tcp://localhost:48400", or empty before the first start.
///
QString SimServer::endpointUrl() const
{
    return _endpointUrl;
}

///
/// \brief Returns the active and recently closed sessions.
/// \return Sessions, active ones first.
///
QList<SessionInfo> SimServer::sessions() const
{
    return _sessions.sessions();
}

///
/// \brief Returns the number of connected clients.
/// \return Active session count.
///
int SimServer::clientCount() const
{
    return _sessions.activeCount();
}

///
/// \brief Returns the wrapped server, for building and browsing its address space.
/// \return QUaServer owned by this object.
///
QUaServer *SimServer::uaServer() const
{
    return _server;
}

///
/// \brief Copies the server description, listener, security and users to QUaServer.
/// \param project Project providing the server description.
/// \param listener Merged endpoint settings.
/// \return False when the security settings cannot be served.
///
bool SimServer::configure(const ProjectConfig &project, const ListenerSettings &listener)
{
    _server->setApplicationName(project.server.applicationName);
    _server->setApplicationUri(project.server.applicationUri);
    _server->setProductName(QStringLiteral(APP_NAME));
    _server->setSoftwareVersion(QStringLiteral(APP_VERSION));
    _server->setMaxSessions(quint16(qBound(1, project.server.maxSessions, kMaxSessionLimit)));
    _server->setHostname(listener.hostname);
    _server->setPort(listener.port);
    _server->setAnonymousLoginAllowed(listener.anonymousAllowed);
    applyUsers(listener);
    _endpointUrl = QStringLiteral("opc.tcp://%1:%2").arg(listener.hostname).arg(listener.port);
    return configureSecurity(listener);
}

///
/// \brief Restricts the offered endpoints to the configured policies and modes.
/// \param listener Merged endpoint settings.
/// \return False when only secure endpoints are configured but none can be offered.
///
bool SimServer::configureSecurity(const ListenerSettings &listener)
{
    const bool wantsSecure = listener.securityModes != QStringList { QStringLiteral("None") };
#ifdef OUASIM_HAVE_ENCRYPTION
    _server->setCertificate(_pki.certificate);
    _server->setPrivateKey(_pki.privateKey);
    // An empty trusted list makes open62541 accept any client certificate.
    _server->setTrustedCertificates(listener.trustedClientsOnly ? _pki.trusted : QList<QByteArray>());
    _server->setIssuerCertificates(_pki.issuers);
    _server->setRejectUntrustedCertificates(listener.trustedClientsOnly);
    _server->setSecurityPolicies(securityPolicies(listener.securityPolicies));
    _server->setSecurityModes(securityModes(listener.securityModes));
    const bool secureAvailable = !_server->certificate().isEmpty() && !_server->privateKey().isEmpty();
    const QString missing = tr("Secure endpoints need a server certificate and private key");
#else
    const bool secureAvailable = false;
    const QString missing = tr("Secure endpoints need a build with encryption support");
#endif
    if (!wantsSecure || secureAvailable)
        return true;
    if (!listener.securityModes.contains(QStringLiteral("None")))
        return fail(missing);
    const QString warning = tr("%1; only unsecured endpoints are offered").arg(missing);
    log(LogEntry::Level::Warning, LogEntry::Category::Security, warning);
    emit startWarning(warning);
    return true;
}

///
/// \brief Replaces the server's user accounts with the ones of the username endpoints.
/// \param listener Merged endpoint settings.
///
void SimServer::applyUsers(const ListenerSettings &listener)
{
    const QStringList existing = _server->userNames();
    for (const QString &name : existing)
        _server->removeUser(name);
    for (const auto &[name, password] : listener.users)
        _server->addUser(name, password);
}

///
/// \brief Reports a failed start.
/// \param reason Explanation for the user.
/// \return Always false, so callers can return it directly.
///
bool SimServer::fail(const QString &reason)
{
    log(LogEntry::Level::Error, LogEntry::Category::Server, reason);
    emit startFailed(reason);
    return false;
}

///
/// \brief Emits an entry produced by the simulator itself.
/// \param level Severity.
/// \param category Producing subsystem.
/// \param message Text.
///
void SimServer::log(LogEntry::Level level, LogEntry::Category category, const QString &message)
{
    emit logAppended(LogMapper::entry(level, category, message));
}

///
/// \brief Forwards a QUaServer log message unless it is trace or debug output.
/// \param log Message emitted by QUaServer.
///
void SimServer::handleLog(const QUaLog &log)
{
    if (const std::optional<LogEntry> entry = LogMapper::fromQUaLog(log))
        emit logAppended(*entry);
}

///
/// \brief Records a client session.
/// \param session Session reported by QUaServer.
///
void SimServer::handleClientConnected(const QUaSession *session)
{
    const SessionInfo info = sessionInfo(session, _endpointUrl);
    _sessions.open(info);
    log(LogEntry::Level::Info, LogEntry::Category::Session,
        tr("Session '%1' opened by %2").arg(info.name, info.user));
    emit sessionsChanged();
}

///
/// \brief Marks a client session closed.
/// \param session Session reported by QUaServer.
///
void SimServer::handleClientDisconnected(const QUaSession *session)
{
    if (!_sessions.close(session->sessionId(), QDateTime::currentDateTime()))
        return;
    log(LogEntry::Level::Info, LogEntry::Category::Session,
        tr("Session '%1' closed").arg(session->applicationName()));
    emit sessionsChanged();
}

///
/// \brief Copies the subscription count and last activity of the open sessions from the server diagnostics.
///        Runs every two seconds while the server runs.
///
void SimServer::refreshSessionDiagnostics()
{
    bool changed = false;
    const QList<const QUaSession *> sessions = _server->sessions();
    for (const QUaSession *session : sessions) {
        const QUaSessionDiagnostics diagnostics = _server->sessionDiagnostics(session);
        if (diagnostics.isValid)
            changed |= _sessions.update(session->sessionId(), int(diagnostics.subscriptionsCount),
                                        diagnostics.lastContactTime.toLocalTime());
    }
    if (changed)
        emit sessionsChanged();
}

#ifdef OUASIM_HAVE_ENCRYPTION
///
/// \brief Forwards a client certificate QUaServer rejected.
/// \param certificate Rejected DER certificate.
/// \param statusCode Reason of the rejection.
///
void SimServer::handleCertificateRejected(const QByteArray &certificate, const QUaStatusCode &statusCode)
{
    emit clientCertificateRejected(certificate, QString::fromLatin1(UA_StatusCode_name(UA_StatusCode(statusCode))));
}
#endif
