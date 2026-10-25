// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedlabel.h
/// \brief Declares a label with the shared single-line overflow behavior.
///

#pragma once

#include <QLabel>

///
/// \brief Plain-text label retaining its full value while eliding the visible text.
///
class ElidedLabel : public QLabel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty plain-text label.
    /// \param parent Parent widget.
    ///
    explicit ElidedLabel(QWidget *parent = nullptr);

    ///
    /// \brief Constructs a plain-text label with an initial value.
    /// \param text Initial value.
    /// \param parent Parent widget.
    ///
    explicit ElidedLabel(const QString &text, QWidget *parent = nullptr);

    ///
    /// \brief Tells whether the single-line value exceeds the available text width.
    /// \return True when the value needs an ellipsis.
    ///
    bool isElided() const;

    ///
    /// \brief Allows a single-line value to shrink to an ellipsis in a layout.
    /// \return Minimum size preserving the label height.
    ///
    QSize minimumSizeHint() const override;

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRect textRect() const;
};
