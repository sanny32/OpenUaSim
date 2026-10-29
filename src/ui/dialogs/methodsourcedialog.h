// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

/// \file methodsourcedialog.h
/// \brief Declares the editor for a method value source.

#pragma once
#include "appbasedialog.h"
#include "data/methodinfo.h"

namespace Ui { class MethodSourceDialog; }

class MethodSourceDialog : public AppBaseDialog
{
    Q_OBJECT
public:
    ///
    /// \brief Edits a source with the current method inputs and available variable references.
    /// \param source Source to edit.
    /// \param method Method supplying input names and namespace URIs.
    /// \param variables Variables offered as sources.
    /// \param parent Owning widget.
    ///
    MethodSourceDialog(const MethodValueSource &source, const MethodInfo &method,
                        const QList<VariableInfo> &variables, QWidget *parent = nullptr);
    /// \brief Releases the source editor.
    ~MethodSourceDialog() override;
    ///
    /// \brief Returns the configured source without applying it to the method.
    /// \return Source using a persistent variable reference.
    ///
    MethodValueSource source() const;
private:
    void updateFields();
    Ui::MethodSourceDialog *ui;
    QStringList _namespaces;
};
