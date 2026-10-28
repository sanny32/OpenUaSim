// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file methodstatuses.cpp
/// \brief Implements the status codes a model method can answer its calls with.
///

#include <utility>

#include "methodstatuses.h"

namespace {

///
/// \brief Returns the offered status codes with their names (OPC UA Part 4, 7.38 and Part 6, A.2).
/// \return Pairs of code and name, in display order.
///
const QList<std::pair<quint32, QString>> &statuses()
{
    static const QList<std::pair<quint32, QString>> list = {
        { 0x00000000, QStringLiteral("Good") },
        { 0x40000000, QStringLiteral("Uncertain") },
        { 0x80000000, QStringLiteral("Bad") },
        { 0x80AB0000, QStringLiteral("BadInvalidArgument") },
        { 0x80AF0000, QStringLiteral("BadInvalidState") },
        { 0x803C0000, QStringLiteral("BadOutOfRange") },
        { 0x801F0000, QStringLiteral("BadUserAccessDenied") },
        { 0x80400000, QStringLiteral("BadNotImplemented") },
        { 0x803D0000, QStringLiteral("BadNotSupported") },
        { 0x80040000, QStringLiteral("BadResourceUnavailable") },
        { 0x800A0000, QStringLiteral("BadTimeout") },
        { 0x808B0000, QStringLiteral("BadDeviceFailure") },
        { 0x80020000, QStringLiteral("BadInternalError") },
    };
    return list;
}

}

namespace MethodStatuses {

///
/// \brief Returns the status codes a model method can answer with.
/// \return Codes in display order, Good first.
///
QList<quint32> codes()
{
    QList<quint32> result;
    for (const auto &[code, name] : statuses())
        result.append(code);
    return result;
}

///
/// \brief Returns the OPC UA name of a status code.
/// \param code Code that codes() lists.
/// \return Name such as "BadInvalidState"; empty for a code codes() does not list.
///
QString name(quint32 code)
{
    for (const auto &[candidate, text] : statuses()) {
        if (candidate == code)
            return text;
    }
    return {};
}

///
/// \brief Parses the OPC UA name of a status code.
/// \param name Name such as "BadInvalidState".
/// \param code Receives the code when the name is known.
/// \return False for a name of a code that codes() does not list.
///
bool fromName(const QString &name, quint32 &code)
{
    for (const auto &[candidate, text] : statuses()) {
        if (text == name) {
            code = candidate;
            return true;
        }
    }
    return false;
}

}
