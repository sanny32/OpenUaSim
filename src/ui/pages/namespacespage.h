// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacespage.h
/// \brief Declares the namespaces page.
///

#pragma once

#include <QList>
#include <QWidget>

#include "data/namespaceinfo.h"

namespace Ui {
class NamespacesPage;
}

class NamespacesModel;
class TreeModel;

///
/// \brief Page listing the namespace array and the details of a namespace.
///
class NamespacesPage : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the page.
    /// \param parent Parent widget.
    ///
    explicit NamespacesPage(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the page.
    ///
    ~NamespacesPage() override;

    ///
    /// \brief Shows a namespace array.
    /// \param namespaces Registered namespaces.
    ///
    void setNamespaces(const QList<NamespaceInfo> &namespaces);

    ///
    /// \brief Returns the namespaces table model.
    /// \return Model of the registered namespaces.
    ///
    const NamespacesModel *namespacesModel() const;

private:
    void showTreeNode(const QModelIndex &current);
    void showNamespace(const QModelIndex &current);

    Ui::NamespacesPage *ui;
    TreeModel *_treeModel;
    NamespacesModel *_namespacesModel;
};
