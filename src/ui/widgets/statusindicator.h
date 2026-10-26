// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file statusindicator.h
/// \brief Declares the coloured dot-and-label status indicator.
///

#pragma once

#include <QWidget>

#include "data/statuskind.h"

///
/// \brief Draws a coloured status dot followed by a label.
///
class StatusIndicator : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an indicator without status or text.
    /// \param parent Parent widget.
    ///
    explicit StatusIndicator(QWidget *parent = nullptr);

    ///
    /// \brief Sets the status and the label shown next to the dot.
    /// \param kind Status selecting the dot colour.
    /// \param text Label text.
    ///
    void setStatus(StatusKind kind, const QString &text);

    ///
    /// \brief Returns the shown status.
    /// \return Status selecting the dot colour.
    ///
    StatusKind status() const;

    ///
    /// \brief Returns the label text.
    /// \return Label text.
    ///
    QString text() const;

    ///
    /// \brief Chooses whether the label is drawn in the status colour.
    /// \param colored True to colour the label, false for the palette text colour.
    ///
    void setColoredText(bool colored);

    ///
    /// \brief Returns the size fitting the dot and the label.
    /// \return Preferred size.
    ///
    QSize sizeHint() const override;

    ///
    /// \brief Returns the preferred size.
    /// \return Same as sizeHint().
    ///
    QSize minimumSizeHint() const override;

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    int dotDiameter() const;
    qreal textAdvance() const;
    QRect textRect() const;

    StatusKind _status = StatusKind::None;
    QString _text;
    bool _coloredText = false;
};
