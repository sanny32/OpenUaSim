// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file variablepropertieswidget.h
/// \brief Declares the editor of the settings and simulation of a variable.
///

#pragma once

#include <QWidget>

#include "data/variableinfo.h"

namespace Ui {
class VariablePropertiesWidget;
}

///
/// \brief Shows a variable with its type and edits its name, data type, access, initial value, quality,
///        historizing and simulation.
///
class VariablePropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the editor.
    /// \param parent Parent widget.
    ///
    explicit VariablePropertiesWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the editor.
    ///
    ~VariablePropertiesWidget() override;

    ///
    /// \brief Shows a variable; a read-only one disables the editor and a model variable its structure.
    /// \param variable Variable as stored.
    ///
    void setVariable(const VariableInfo &variable);

    ///
    /// \brief Shows a live quality change without discarding edited properties.
    /// \param nodeId Variable whose current quality changed.
    /// \param quality Quality reported by the server.
    ///
    void updateQuality(const QString &nodeId, VariableInfo::Quality quality);

    ///
    /// \brief Empties and disables the editor.
    ///
    void clear();

signals:
    ///
    /// \brief Asks to apply the edited settings.
    /// \param variable Shown variable with the edited fields, as editedVariable() reads them.
    ///
    void applyRequested(const VariableInfo &variable);

    ///
    /// \brief Asks to edit the alarm of the shown variable.
    /// \param variable Variable as stored, with its current alarm settings.
    ///
    void alarmEditRequested(const VariableInfo &variable);

protected:
    void changeEvent(QEvent *event) override;

private:
    void fillCombos();
    void refreshQualityIndicator();
    void retranslate();
    void updateSimulationFields();
    void requestApply();
    void requestAlarmEdit();
    void revert();
    VariableInfo editedVariable() const;

    Ui::VariablePropertiesWidget *ui;
    VariableInfo _variable;
};
