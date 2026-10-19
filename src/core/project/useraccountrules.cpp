// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file useraccountrules.cpp
/// \brief Implements the validation of the project's user accounts.
///

#include <QCoreApplication>
#include <QSet>

#include "useraccountrules.h"

namespace UserAccountRules {

///
/// \brief Returns why a list of user accounts cannot be served.
/// \param users Accounts to check.
/// \return Problem text, or empty when every account has a user name that no other account uses.
///
QString problem(const QList<UserAccount> &users)
{
    QSet<QString> names;
    for (const UserAccount &user : users) {
        if (user.username.trimmed().isEmpty())
            return QCoreApplication::translate("UserAccountRules", "A user account has no user name");
        if (names.contains(user.username))
            return QCoreApplication::translate("UserAccountRules", "The user name '%1' is used more than once")
                .arg(user.username);
        names.insert(user.username);
    }
    return {};
}

}
