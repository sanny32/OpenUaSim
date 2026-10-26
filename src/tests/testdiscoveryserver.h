// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file testdiscoveryserver.h
/// \brief Declares a Local Discovery Server for tests of the server registration.
///

#pragma once

#include <thread>

#include <QStringList>

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "testport.h"

///
/// \brief Local Discovery Server on a free port, run by open62541 in its own thread so it
///        answers while the server under test blocks the test thread on stop.
///
class TestDiscoveryServer
{
public:
    TestDiscoveryServer() = default;
    TestDiscoveryServer(const TestDiscoveryServer &) = delete;
    TestDiscoveryServer &operator=(const TestDiscoveryServer &) = delete;

    ~TestDiscoveryServer()
    {
        if (!_server)
            return;
        _running = false;
        _thread.join();
        UA_Server_delete(_server);
    }

    ///
    /// \brief Starts listening; with a DER \a certificate and \a privateKey it also offers encrypted endpoints and
    ///        accepts only the DER client certificates in \a trusted.
    /// \return False when the server could not be created.
    ///
    bool start(const QByteArray &certificate = QByteArray(), const QByteArray &privateKey = QByteArray(),
               const QList<QByteArray> &trusted = QList<QByteArray>())
    {
        _port = TestPort::freePort();
        UA_ServerConfig config;
        memset(&config, 0, sizeof(UA_ServerConfig));
        config.logging = UA_Log_Stdout_new(UA_LOGLEVEL_WARNING);
        UA_StatusCode status = UA_STATUSCODE_GOOD;
#ifdef UA_ENABLE_ENCRYPTION
        if (!certificate.isEmpty()) {
            const UA_ByteString cert = byteString(certificate);
            const UA_ByteString key = byteString(privateKey);
            QList<UA_ByteString> trustList;
            for (const QByteArray &bytes : trusted)
                trustList << byteString(bytes);
            status = UA_ServerConfig_setDefaultWithSecurityPolicies(&config, _port, &cert, &key, trustList.constData(),
                                                                    size_t(trustList.size()), nullptr, 0, nullptr, 0);
        } else
#endif
        {
            Q_UNUSED(certificate);
            Q_UNUSED(privateKey);
            Q_UNUSED(trusted);
            status = UA_ServerConfig_setMinimal(&config, _port, nullptr);
        }
        if (status != UA_STATUSCODE_GOOD) {
            UA_ServerConfig_clear(&config);
            return false;
        }
        config.applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
        _server = UA_Server_newWithConfig(&config);
        if (!_server)
            return false;
        _running = true;
        _thread = std::thread([this] { UA_Server_run(_server, &_running); });
        return true;
    }

    ///
    /// \brief Returns the URL servers register at.
    ///
    QString url() const { return QStringLiteral("opc.tcp://127.0.0.1:%1").arg(_port); }

    ///
    /// \brief Asks the discovery server with FindServers for the servers registered online.
    /// \return Application URIs of the registered servers.
    ///
    QStringList registeredServers() const
    {
        UA_ClientConfig config;
        memset(&config, 0, sizeof(UA_ClientConfig));
        config.logging = UA_Log_Stdout_new(UA_LOGLEVEL_WARNING);
        UA_ClientConfig_setDefault(&config);
        UA_Client *client = UA_Client_newWithConfig(&config);
        size_t serversSize = 0;
        UA_ApplicationDescription *servers = nullptr;
        QStringList uris;
        if (UA_Client_findServers(client, url().toUtf8().constData(), 0, nullptr, 0, nullptr, &serversSize, &servers)
            == UA_STATUSCODE_GOOD) {
            for (size_t i = 0; i < serversSize; ++i) {
                if (servers[i].applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER)
                    continue;
                const UA_String &uri = servers[i].applicationUri;
                uris << QString::fromUtf8(reinterpret_cast<const char *>(uri.data), int(uri.length));
            }
        }
        UA_Array_delete(servers, serversSize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
        UA_Client_delete(client);
        return uris;
    }

private:
    static UA_ByteString byteString(const QByteArray &bytes)
    {
        UA_ByteString string;
        string.length = size_t(bytes.size());
        string.data = reinterpret_cast<UA_Byte *>(const_cast<char *>(bytes.constData()));
        return string;
    }

    UA_Server *_server = nullptr;
    quint16 _port = 0;
    volatile UA_Boolean _running = false;
    std::thread _thread;
};
