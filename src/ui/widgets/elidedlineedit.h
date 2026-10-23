// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedlineedit.h
/// \brief Declares a line edit that elides text it cannot fit.
///

#pragma once

#include <QLineEdit>

///
/// \brief Line edit that, while unfocused, shows its text from the beginning with a trailing ellipsis
///        when it does not fit, and the full text as a tooltip.
///
class ElidedLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty line edit.
    /// \param parent Parent widget.
    ///
    explicit ElidedLineEdit(QWidget *parent = nullptr);

    ///
    /// \brief Tells whether the text is wider than the space shown for it.
    /// \return True when the text is clipped.
    ///
    bool isElided() const;

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRect textRect() const;
    void showStart();
};
