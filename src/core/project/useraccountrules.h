// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file useraccountrules.h
/// \brief Declares the validation of the project's user accounts.
///

#pragma once

#include <QList>
#include <QString>

#include "data/projectconfig.h"

namespace UserAccountRules {

///
/// \brief Returns why a list of user accounts cannot be served.
/// \param users Accounts to check.
/// \return Problem text, or empty when every account has a user name that no other account uses.
///
QString problem(const QList<UserAccount> &users);

}
