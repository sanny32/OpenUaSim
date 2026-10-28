// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file alarmdialog.h
/// \brief Declares the dialog that edits the alarm of a variable.
///

#pragma once

#include "appbasedialog.h"

#include "data/variableinfo.h"

namespace Ui {
class AlarmDialog;
}

///
/// \brief Edits the limits or normal value and the severity of a variable's alarm.
///
class AlarmDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog for a variable.
    /// \param variable Variable whose alarm is edited; its data type selects limits or a normal value.
    /// \param parent Parent widget.
    ///
    explicit AlarmDialog(const VariableInfo &variable, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog.
    ///
    ~AlarmDialog() override;

    ///
    /// \brief Returns the edited alarm settings.
    /// \return Settings; a disabled alarm keeps the entered values for later.
    ///
    AlarmSettings alarm() const;

private:
    void validate();

    Ui::AlarmDialog *ui;
    QString _dataType;
    int _arrayLength = 0;
};
