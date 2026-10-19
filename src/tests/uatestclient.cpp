// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file uatestclient.cpp
/// \brief Implements a blocking OPC UA client for tests that talk to an in-process server.
///

#include <atomic>
#include <cstring>
#include <thread>

#include <QCoreApplication>

#include <QUaServer>
#include <quatypesconverter.h>

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include "uatestclient.h"

///
/// \brief Creates a client that logs only warnings and errors.
///
UaTestClient::UaTestClient()
{
    UA_ClientConfig config;
    std::memset(&config, 0, sizeof(UA_ClientConfig));
    config.logging = UA_Log_Stdout_new(UA_LOGLEVEL_WARNING);
    UA_ClientConfig_setDefault(&config);
    config.timeout = 5000;
    // Test servers have no certificate, so user names travel over SecurityPolicy None.
    config.allowNonePolicyPassword = true;
    _client = UA_Client_newWithConfig(&config);
}

///
/// \brief Disconnects and releases the client.
///
UaTestClient::~UaTestClient()
{
    disconnect();
    UA_Client_delete(_client);
}

///
/// \brief Opens an anonymous session.
/// \param url Endpoint URL.
/// \return Service result.
///
UA_StatusCode UaTestClient::connect(const QString &url)
{
    const QByteArray endpoint = url.toUtf8();
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    runInWorker([&] { status = UA_Client_connect(_client, endpoint.constData()); });
    return status;
}

///
/// \brief Opens a session authenticated with a user name and password.
/// \param url Endpoint URL.
/// \param userName User name.
/// \param password Password.
/// \return Service result.
///
UA_StatusCode UaTestClient::connectUsername(const QString &url, const QString &userName, const QString &password)
{
    const QByteArray endpoint = url.toUtf8();
    const QByteArray user = userName.toUtf8();
    const QByteArray pass = password.toUtf8();
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    runInWorker([&] {
        status = UA_Client_connectUsername(_client, endpoint.constData(), user.constData(), pass.constData());
    });
    return status;
}

///
/// \brief Closes the session and the secure channel, if any.
/// \return Service result.
///
UA_StatusCode UaTestClient::disconnect()
{
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    runInWorker([&] { status = UA_Client_disconnect(_client); });
    return status;
}

///
/// \brief Makes the following connections use a SignAndEncrypt secure channel; encryption builds only.
/// \param certificate DER client certificate.
/// \param privateKey DER private key of the certificate.
/// \param applicationUri Application URI stored in the certificate.
/// \return Status of the client configuration.
///
UA_StatusCode UaTestClient::setEncryption(const QByteArray &certificate, const QByteArray &privateKey,
                                          const QString &applicationUri)
{
#ifdef UA_ENABLE_ENCRYPTION
    UA_ClientConfig *config = UA_Client_getConfig(_client);
    UA_ByteString uaCertificate { static_cast<size_t>(certificate.size()),
                                  reinterpret_cast<UA_Byte *>(const_cast<char *>(certificate.constData())) };
    UA_ByteString uaPrivateKey { static_cast<size_t>(privateKey.size()),
                                 reinterpret_cast<UA_Byte *>(const_cast<char *>(privateKey.constData())) };
    const UA_StatusCode status =
        UA_ClientConfig_setDefaultEncryption(config, uaCertificate, uaPrivateKey, nullptr, 0, nullptr, 0);
    if (status != UA_STATUSCODE_GOOD)
        return status;
    config->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    // The server rejects a client whose application URI differs from the one in its certificate.
    UA_String_clear(&config->clientDescription.applicationUri);
    config->clientDescription.applicationUri = UA_String_fromChars(applicationUri.toUtf8().constData());
    return UA_STATUSCODE_GOOD;
#else
    Q_UNUSED(certificate);
    Q_UNUSED(privateKey);
    Q_UNUSED(applicationUri);
    return UA_STATUSCODE_BADNOTSUPPORTED;
#endif
}

///
/// \brief Reads the Value attribute of a node.
/// \param nodeId Node id in XML notation, e.g. "ns=1;s=Motor_01.Speed".
/// \param value Receives the value on success.
/// \return Service result.
///
UA_StatusCode UaTestClient::readValue(const QString &nodeId, QVariant &value)
{
    UA_NodeId id = QUaNodeId(nodeId).toUaNodeId();
    UA_Variant uaValue;
    UA_Variant_init(&uaValue);
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    runInWorker([&] { status = UA_Client_readValueAttribute(_client, id, &uaValue); });
    if (status == UA_STATUSCODE_GOOD)
        value = QUaTypesConverter::uaVariantToQVariant(uaValue);
    UA_Variant_clear(&uaValue);
    UA_NodeId_clear(&id);
    return status;
}

///
/// \brief Writes the Value attribute of a node.
/// \param nodeId Node id in XML notation.
/// \param value Value to write; its Qt type selects the OPC UA type.
/// \return Service result.
///
UA_StatusCode UaTestClient::writeValue(const QString &nodeId, const QVariant &value)
{
    UA_NodeId id = QUaNodeId(nodeId).toUaNodeId();
    UA_Variant uaValue = QUaTypesConverter::uaVariantFromQVariant(value);
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    runInWorker([&] { status = UA_Client_writeValueAttribute(_client, id, &uaValue); });
    UA_Variant_clear(&uaValue);
    UA_NodeId_clear(&id);
    return status;
}

///
/// \brief Creates a subscription with a data change monitored item on the Value of a node.
/// \param nodeId Node id in XML notation.
/// \return Status of the monitored item creation.
///
UA_StatusCode UaTestClient::monitorValue(const QString &nodeId)
{
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    runInWorker([&] {
        const UA_CreateSubscriptionResponse subscription = UA_Client_Subscriptions_create(
            _client, UA_CreateSubscriptionRequest_default(), nullptr, nullptr, nullptr);
        status = subscription.responseHeader.serviceResult;
        if (status != UA_STATUSCODE_GOOD)
            return;
        UA_NodeId id = QUaNodeId(nodeId).toUaNodeId();
        const UA_MonitoredItemCreateResult item = UA_Client_MonitoredItems_createDataChange(
            _client, subscription.subscriptionId, UA_TIMESTAMPSTORETURN_NEITHER,
            UA_MonitoredItemCreateRequest_default(id), nullptr, nullptr, nullptr);
        status = item.statusCode;
        UA_NodeId_clear(&id);
    });
    return status;
}

///
/// \brief Runs a blocking client call on a worker thread while processing events here.
/// \param job Client call.
///
void UaTestClient::runInWorker(const std::function<void()> &job)
{
    std::atomic_bool done { false };
    std::thread worker([&] {
        job();
        done = true;
    });
    while (!done)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    worker.join();
}
