// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file propertynodewidget.h
/// \brief Declares the editor of a property of a variable.
///

#pragma once

#include <QWidget>

#include "data/variableinfo.h"

namespace Ui {
class PropertyNodeWidget;
}

///
/// \brief Shows a property of a variable, such as its EURange, with its type, data type and access, and edits its
///        value.
///
class PropertyNodeWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the editor.
    /// \param parent Parent widget.
    ///
    explicit PropertyNodeWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the editor.
    ///
    ~PropertyNodeWidget() override;

    ///
    /// \brief Shows a property; its value text is the initial value, editable unless the property is read-only, and a
    ///        Range is edited as its Low and High limits.
    /// \param property Property as listed.
    ///
    void setNode(const VariableInfo &property);

    ///
    /// \brief Empties and disables the editor.
    ///
    void clear();

signals:
    ///
    /// \brief Asks to apply the edited value.
    /// \param property Shown property with the typed value text as initial value, "Low; High" for a Range; invalid when
    ///        it was not edited.
    ///
    void applyRequested(const VariableInfo &property);

protected:
    void changeEvent(QEvent *event) override;

private:
    void requestApply();
    void revert();
    QVariant typedValue() const;

    Ui::PropertyNodeWidget *ui;
    VariableInfo _property;
};
