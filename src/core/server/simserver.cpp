// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file simserver.cpp
/// \brief Implements the OPC UA server the simulator runs.
///

#include <utility>

#include <QSslCertificate>
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
/// \param certificateUsers User names that logged in with an X.509 certificate.
/// \return Active session description.
///
SessionInfo sessionInfo(const QUaSession *session, const QString &endpointUrl, const QSet<QString> &certificateUsers)
{
    SessionInfo info;
    info.sessionId = session->sessionId();
    info.name = session->sessionName();
    info.client = session->productUri().isEmpty() ? session->applicationName() : session->productUri();
    info.user = session->isAnonymous() ? QObject::tr("Anonymous") : session->userName();
    info.active = true;
    info.created = session->timestamp().isValid() ? session->timestamp().toLocalTime() : QDateTime::currentDateTime();
    info.lastActivity = info.created;
    info.applicationUri = session->applicationUri();
    info.endpointUrl = session->endpointUrl().isEmpty() ? endpointUrl : session->endpointUrl();
    info.securityMode = securityModeName(session->securityMode());
    if (session->isAnonymous())
        info.userIdentity = QStringLiteral("Anonymous");
    else if (certificateUsers.contains(session->userName()))
        info.userIdentity = QStringLiteral("Certificate");
    else
        info.userIdentity = QStringLiteral("Username");
    return info;
}

///
/// \brief Describes the subscriptions of a session from the server diagnostics.
/// \param server Server holding the session.
/// \param session Session to describe.
/// \return Subscriptions in the order the server lists them.
///
QList<SubscriptionInfo> subscriptionsOf(const QUaServer &server, const QUaSession *session)
{
    QList<SubscriptionInfo> result;
    const QList<QUaSubscriptionDiagnostics> subscriptions = server.subscriptionDiagnostics(session);
    for (const QUaSubscriptionDiagnostics &subscription : subscriptions) {
        SubscriptionInfo info;
        info.id = subscription.subscriptionId;
        info.publishingIntervalMs = subscription.publishingInterval;
        info.publishingEnabled = subscription.publishingEnabled;
        info.priority = subscription.priority;
        info.monitoredItems = subscription.monitoredItemCount;
        info.notifications = subscription.notificationsCount;
        info.maxKeepAliveCount = subscription.maxKeepAliveCount;
        info.maxLifetimeCount = subscription.maxLifetimeCount;
        result.append(info);
    }
    return result;
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
/// \brief Replaces the server with an empty one, dropping every node and namespace loaded into it and the value
///        history. A running server is stopped first; the next start() serves the new one.
///
void SimServer::reset()
{
    stop();
    releaseServer();
    _history.clear();
    createServer();
}

///
/// \brief Creates the QUaServer on the value history and forwards its log, session and certificate events.
///
void SimServer::createServer()
{
    _server = new QUaServer(this);
    _server->setHistorizer(_history);
    connect(_server, &QUaServer::logMessage, this, &SimServer::handleLog);
    connect(_server, &QUaServer::clientConnected, this, &SimServer::handleClientConnected);
    connect(_server, &QUaServer::clientDisconnected, this, &SimServer::handleClientDisconnected);
    connect(_server, &QUaServer::discoveryRegistrationFinished, this, &SimServer::handleDiscoveryRegistration);
#ifdef OUASIM_HAVE_ENCRYPTION
    connect(_server, &QUaServer::clientCertificateRejected, this, &SimServer::handleCertificateRejected);
    connect(_server, &QUaServer::discoveryServerCertificateRejected, this,
            &SimServer::handleDiscoveryCertificateRejected);
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
    if (listener.usernameAllowed && project.users.isEmpty())
        return fail(tr("Endpoints accept user names but the project has no user accounts"));
    if (!configure(project, listener))
        return false;

    if (!_server->start())
        return fail(tr("The server could not listen on port %1; it may be in use by another program")
                        .arg(listener.port));

    _sessions.clear();
    _discoveryStatus.clear();
    _startTime = QDateTime::currentDateTime();
    log(LogEntry::Level::Info, LogEntry::Category::Server, tr("Server started on %1").arg(_endpointUrl));
    emit sessionsChanged();
    _diagnosticsTimer->start();
    emit runningChanged(true);
    return true;
}

///
/// \brief Sets the certificates: a running server applies the trusted and issuer certificates at once and
///        the server certificate on the next start; ignored in builds without encryption.
/// \param material Server certificate and key with the trusted and issuer certificates.
/// \return False when the server runs with another server certificate and needs a restart.
///
bool SimServer::setPkiMaterial(const PkiMaterial &material)
{
    if (!_server->isRunning()) {
        _pki = material;
        return true;
    }
    _pki.trusted = material.trusted;
    _pki.issuers = material.issuers;
    applyTrustLists();
    return material.certificate == _pki.certificate && material.privateKey == _pki.privateKey;
}

///
/// \brief Gives QUaServer the trusted and issuer certificates; a running server applies them to the next client
///        and the next registration at the discovery server.
///
void SimServer::applyTrustLists()
{
#ifdef OUASIM_HAVE_ENCRYPTION
    // An empty trusted list makes open62541 accept any client certificate.
    _server->setTrustedCertificates(_trustedClientsOnly ? _pki.trusted : QList<QByteArray>());
    _server->setDiscoveryServerTrustedCertificates(_pki.trusted);
    _server->setIssuerCertificates(_pki.issuers);
#endif
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
    _history.setLimits(project.server.history);
    _server->setHostname(listener.hostname);
    _server->setPort(listener.port);
    _server->setAnonymousLoginAllowed(listener.anonymousAllowed);
    const DiscoveryRegistration &discovery = project.server.discovery;
    _server->setDiscoveryServerUrl(discovery.enabled ? discovery.url.trimmed() : QString());
    _server->setDiscoveryRegistrationInterval(discovery.intervalSeconds * 1000);
#ifdef OUASIM_HAVE_ENCRYPTION
    _server->setVerifyDiscoveryServerCertificate(discovery.verifyCertificate);
#endif
    applyUsers(listener.usernameAllowed ? project.users : QList<UserAccount>());
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
    _trustedClientsOnly = listener.trustedClientsOnly;
    applyTrustLists();
    _server->setRejectUntrustedCertificates(listener.trustedClientsOnly);
    _server->setSecurityPolicies(securityPolicies(listener.securityPolicies));
    _server->setSecurityModes(securityModes(listener.securityModes));
    _certificateUsers.clear();
    if (listener.userCertificateAllowed)
        _server->setUserCertificateCallback([this](const QByteArray &certificate) {
            return certificateUserName(certificate);
        });
    else
        _server->setUserCertificateCallback();
    const bool secureAvailable = !_server->certificate().isEmpty() && !_server->privateKey().isEmpty();
    const QString missing = tr("Secure endpoints need a server certificate and private key");
    if (listener.userCertificateAllowed && !secureAvailable)
        log(LogEntry::Level::Warning, LogEntry::Category::Security,
            tr("User certificates need a server certificate and private key; they are not accepted"));
#else
    const bool secureAvailable = false;
    const QString missing = tr("Secure endpoints need a build with encryption support");
    if (listener.userCertificateAllowed)
        log(LogEntry::Level::Warning, LogEntry::Category::Security,
            tr("User certificates need a build with encryption support; they are not accepted"));
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

#ifdef OUASIM_HAVE_ENCRYPTION
///
/// \brief Names the user of an X.509 certificate that open62541 already checked against the trust list.
/// \param certificate DER user certificate.
/// \return Common name, or another subject field when it has none; empty, which rejects the user, for an
///         unreadable certificate.
///
QString SimServer::certificateUserName(const QByteArray &certificate)
{
    const QString name = QSslCertificate(certificate, QSsl::Der).subjectDisplayName();
    if (!name.isEmpty())
        _certificateUsers.insert(name);
    return name;
}
#endif

///
/// \brief Replaces the server's user accounts.
/// \param users Accounts to accept; empty when no endpoint takes username tokens.
///
void SimServer::applyUsers(const QList<UserAccount> &users)
{
    const QStringList existing = _server->userNames();
    for (const QString &name : existing)
        _server->removeUser(name);
    for (const UserAccount &user : users)
        _server->addUser(user.username, user.password);
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
    const SessionInfo info = sessionInfo(session, _endpointUrl, _certificateUsers);
    _sessions.open(info);
    log(LogEntry::Level::Info, LogEntry::Category::Session,
        tr("Session '%1' opened by %2").arg(info.name, info.user));
    emit sessionsChanged();
    // With trusted clients only, an accepted certificate passed the trust lists, possibly through a trusted CA.
    if (!_trustedClientsOnly && !session->clientCertificate().isEmpty())
        emit unverifiedClientConnected(session->clientCertificate());
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
/// \brief Logs a registration at the discovery server when its outcome changes, so renewals stay quiet, and the
///        removal of a registration that succeeded.
/// \param isOnline True for a registration, false for its removal on stop.
/// \param statusCode Answer of the discovery server.
///
void SimServer::handleDiscoveryRegistration(bool isOnline, const QUaStatusCode &statusCode)
{
    const QString status = QString::fromLatin1(UA_StatusCode_name(UA_StatusCode(statusCode)));
    const bool good = UA_StatusCode(statusCode) == UA_STATUSCODE_GOOD;
    const bool untrusted = std::exchange(_discoveryCertificateRejected, false);
    const QString url = _server->discoveryServerUrl();
    if (!isOnline) {
        if (_discoveryStatus != QLatin1String("Good"))
            return;
        _discoveryStatus.clear();
        if (good)
            log(LogEntry::Level::Info, LogEntry::Category::Server,
                tr("Unregistered from the discovery server %1").arg(url));
        else
            log(LogEntry::Level::Warning, LogEntry::Category::Server,
                tr("Could not unregister from the discovery server %1: %2").arg(url, status));
        return;
    }
    if (status == _discoveryStatus)
        return;
    _discoveryStatus = status;
    if (good)
        log(LogEntry::Level::Info, LogEntry::Category::Server, tr("Registered at the discovery server %1").arg(url));
    else if (UA_StatusCode(statusCode) == UA_STATUSCODE_BADSECURITYCHECKSFAILED)
        log(LogEntry::Level::Warning, LogEntry::Category::Server,
            tr("The discovery server %1 rejected the server certificate (%2); trust it on the discovery server")
                .arg(url, status));
    else if (untrusted)
        log(LogEntry::Level::Warning, LogEntry::Category::Server,
            tr("The server does not trust the certificate of the discovery server %1 (%2); "
               "trust it on the Certificates page")
                .arg(url, status));
    else
        log(LogEntry::Level::Warning, LogEntry::Category::Server,
            tr("Could not register at the discovery server %1: %2").arg(url, status));
}

///
/// \brief Copies the subscriptions and last activity of the open sessions from the server diagnostics.
///        Runs every two seconds while the server runs.
///
void SimServer::refreshSessionDiagnostics()
{
    bool changed = false;
    const QList<const QUaSession *> sessions = _server->sessions();
    for (const QUaSession *session : sessions) {
        const QUaSessionDiagnostics diagnostics = _server->sessionDiagnostics(session);
        if (diagnostics.isValid)
            changed |= _sessions.update(session->sessionId(), subscriptionsOf(*_server, session),
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

///
/// \brief Forwards a discovery server certificate QUaServer rejected; the registration result that follows
///        explains the failure with it.
/// \param certificate Rejected DER certificate.
/// \param statusCode Reason of the rejection.
///
void SimServer::handleDiscoveryCertificateRejected(const QByteArray &certificate, const QUaStatusCode &statusCode)
{
    _discoveryCertificateRejected = true;
    emit discoveryServerCertificateRejected(certificate,
                                            QString::fromLatin1(UA_StatusCode_name(UA_StatusCode(statusCode))));
}
#endif
