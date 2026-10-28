// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file methodpropertieswidget.h
/// \brief Declares the editor of a method.
///

#pragma once

#include <QWidget>

#include "data/methodinfo.h"

namespace Ui {
class MethodPropertiesWidget;
}

class QTableWidget;
class QTableWidgetItem;

///
/// \brief Shows a method with its arguments and edits the response a method of an imported model gives clients.
///
class MethodPropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the editor.
    /// \param parent Parent widget.
    ///
    explicit MethodPropertiesWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the editor.
    ///
    ~MethodPropertiesWidget() override;

    ///
    /// \brief Shows a method; the response status and output values are editable for a method of an imported model.
    /// \param method Method as described.
    ///
    void setMethod(const MethodInfo &method);

    ///
    /// \brief Empties and disables the editor.
    ///
    void clear();

signals:
    ///
    /// \brief Asks to apply the edited response.
    /// \param method Shown method with the chosen status and the typed output values.
    ///
    void applyRequested(const MethodInfo &method);

protected:
    void changeEvent(QEvent *event) override;

private:
    void fillInputs(const QList<ArgumentInfo> &arguments);
    void fillOutputs(const QList<ArgumentInfo> &arguments, const QStringList &values);
    static int addArgumentRow(QTableWidget *table, const ArgumentInfo &argument);
    QTableWidgetItem *valueItem(const ArgumentInfo &argument, const QString &value) const;
    void showAction();
    void markEdited();
    void requestApply();
    void revert();

    Ui::MethodPropertiesWidget *ui;
    MethodInfo _method;
    bool _filling = false;
};
