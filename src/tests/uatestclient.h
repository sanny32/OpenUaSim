// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file uatestclient.h
/// \brief Declares a blocking OPC UA client for tests that talk to an in-process server.
///

#pragma once

#include <functional>

#include <QString>
#include <QVariant>

#include <open62541/client.h>

///
/// \brief Blocking open62541 client; requests run on a worker thread while the test thread keeps
///        processing the events the in-process QUaServer iterates on.
///
class UaTestClient
{
public:
    ///
    /// \brief Creates a client that logs only warnings and errors.
    ///
    UaTestClient();

    ///
    /// \brief Disconnects and releases the client.
    ///
    ~UaTestClient();

    UaTestClient(const UaTestClient &) = delete;
    UaTestClient &operator=(const UaTestClient &) = delete;

    ///
    /// \brief Opens an anonymous session.
    /// \param url Endpoint URL.
    /// \return Service result.
    ///
    UA_StatusCode connect(const QString &url);

    ///
    /// \brief Opens a session authenticated with a user name and password.
    /// \param url Endpoint URL.
    /// \param userName User name.
    /// \param password Password.
    /// \return Service result.
    ///
    UA_StatusCode connectUsername(const QString &url, const QString &userName, const QString &password);

    ///
    /// \brief Closes the session and the secure channel, if any.
    /// \return Service result.
    ///
    UA_StatusCode disconnect();

    ///
    /// \brief Makes the following connections use a SignAndEncrypt secure channel; encryption builds only.
    /// \param certificate DER client certificate.
    /// \param privateKey DER private key of the certificate.
    /// \param applicationUri Application URI stored in the certificate.
    /// \return Status of the client configuration.
    ///
    UA_StatusCode setEncryption(const QByteArray &certificate, const QByteArray &privateKey,
                                const QString &applicationUri);

    ///
    /// \brief Reads the Value attribute of a node.
    /// \param nodeId Node id in XML notation, e.g. "ns=1;s=Motor_01.Speed".
    /// \param value Receives the value on success.
    /// \return Service result.
    ///
    UA_StatusCode readValue(const QString &nodeId, QVariant &value);

    ///
    /// \brief Writes the Value attribute of a node.
    /// \param nodeId Node id in XML notation.
    /// \param value Value to write; its Qt type selects the OPC UA type.
    /// \return Service result.
    ///
    UA_StatusCode writeValue(const QString &nodeId, const QVariant &value);

    ///
    /// \brief Calls a method of an object.
    /// \param objectId Node id of the object.
    /// \param methodId Node id of the method.
    /// \param inputs Input arguments; their Qt types select the OPC UA types.
    /// \param output Receives the first output argument.
    /// \return Service result.
    ///
    UA_StatusCode callMethod(const QString &objectId, const QString &methodId, const QVariantList &inputs,
                             QVariant &output);

    ///
    /// \brief Creates a subscription with a data change monitored item on the Value of a node.
    /// \param nodeId Node id in XML notation.
    /// \return Status of the monitored item creation.
    ///
    UA_StatusCode monitorValue(const QString &nodeId);

private:
    void runInWorker(const std::function<void()> &job);

    UA_Client *_client;
};
