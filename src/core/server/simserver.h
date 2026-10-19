// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file simserver.h
/// \brief Declares the OPC UA server the simulator runs.
///

#pragma once

#include <QDateTime>
#include <QObject>

#include "data/logentry.h"
#include "data/projectconfig.h"
#include "data/sessioninfo.h"
#include "pki/certificatestore.h"
#include "sessiontracker.h"

class QTimer;
class QUaServer;
class QUaSession;
class QUaStatusCode;
struct ListenerSettings;
struct QUaLog;

///
/// \brief Owns the QUaServer, applies a project's endpoints to it and reports its log and sessions.
///
class SimServer : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Creates a stopped server.
    /// \param parent Owning QObject.
    ///
    explicit SimServer(QObject *parent = nullptr);

    ///
    /// \brief Stops the server and releases it.
    ///
    ~SimServer() override;

    ///
    /// \brief Applies the project's server settings and endpoints and starts listening.
    /// \param project Project to serve; its nodes are built separately.
    /// \return True when the server runs; otherwise startFailed() tells why.
    ///
    bool start(const ProjectConfig &project);

    ///
    /// \brief Sets the certificates applied on the next start; ignored in builds without encryption.
    /// \param material Server certificate and key with the trusted and issuer certificates.
    ///
    void setPkiMaterial(const PkiMaterial &material);

    ///
    /// \brief Stops listening and closes every session; does nothing when stopped.
    ///
    void stop();

    ///
    /// \brief Tells whether the server is listening.
    /// \return True between a successful start() and stop().
    ///
    bool isRunning() const;

    ///
    /// \brief Returns when the server last started.
    /// \return Start time, or an invalid time when it never ran.
    ///
    QDateTime startTime() const;

    ///
    /// \brief Returns the URL clients connect to.
    /// \return URL such as "opc.tcp://localhost:48400", or empty before the first start.
    ///
    QString endpointUrl() const;

    ///
    /// \brief Returns the active and recently closed sessions.
    /// \return Sessions, active ones first.
    ///
    QList<SessionInfo> sessions() const;

    ///
    /// \brief Returns the number of connected clients.
    /// \return Active session count.
    ///
    int clientCount() const;

    ///
    /// \brief Returns the wrapped server, for building and browsing its address space.
    /// \return QUaServer owned by this object.
    ///
    QUaServer *uaServer() const;

    ///
    /// \brief Copies the subscription count and last activity of the open sessions from the server diagnostics.
    ///        Runs every two seconds while the server runs.
    ///
    void refreshSessionDiagnostics();

signals:
    ///
    /// \brief Announces that the server started or stopped.
    /// \param running New state.
    ///
    void runningChanged(bool running);

    ///
    /// \brief Announces a new server log entry.
    /// \param entry Entry to append.
    ///
    void logAppended(const LogEntry &entry);

    ///
    /// \brief Announces that a session opened or closed.
    ///
    void sessionsChanged();

    ///
    /// \brief Announces that start() failed.
    /// \param reason Explanation for the user.
    ///
    void startFailed(const QString &reason);

    ///
    /// \brief Announces that start() serves less than configured, e.g. drops secure endpoints.
    /// \param reason Explanation for the user.
    ///
    void startWarning(const QString &reason);

    ///
    /// \brief Announces a client application certificate that failed validation; encryption builds only.
    /// \param certificate Rejected DER certificate.
    /// \param reason Status code name, e.g. BadCertificateUntrusted.
    ///
    void clientCertificateRejected(const QByteArray &certificate, const QString &reason);

private:
    bool configure(const ProjectConfig &project, const ListenerSettings &listener);
    bool configureSecurity(const ListenerSettings &listener);
    void applyUsers(const ListenerSettings &listener);
    bool fail(const QString &reason);
    void log(LogEntry::Level level, LogEntry::Category category, const QString &message);
    void handleLog(const QUaLog &log);
    void handleClientConnected(const QUaSession *session);
    void handleClientDisconnected(const QUaSession *session);
#ifdef OUASIM_HAVE_ENCRYPTION
    void handleCertificateRejected(const QByteArray &certificate, const QUaStatusCode &statusCode);
#endif

    QUaServer *_server;
    QTimer *_diagnosticsTimer;
    SessionTracker _sessions;
    QDateTime _startTime;
    QString _endpointUrl;
    PkiMaterial _pki;
};
