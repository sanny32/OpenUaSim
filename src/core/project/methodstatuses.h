// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file methodstatuses.h
/// \brief Declares the status codes a model method can answer its calls with.
///

#pragma once

#include <QList>
#include <QString>

namespace MethodStatuses {

///
/// \brief Returns the status codes a model method can answer with.
/// \return Codes in display order, Good first.
///
QList<quint32> codes();

///
/// \brief Returns the OPC UA name of a status code.
/// \param code Code that codes() lists.
/// \return Name such as "BadInvalidState"; empty for a code codes() does not list.
///
QString name(quint32 code);

///
/// \brief Parses the OPC UA name of a status code.
/// \param name Name such as "BadInvalidState".
/// \param code Receives the code when the name is known.
/// \return False for a name of a code that codes() does not list.
///
bool fromName(const QString &name, quint32 &code);

}
