// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file elidedcombobox.h
/// \brief Declares a combo box with the same overflow behavior as other text fields.
///

#pragma once

#include <QComboBox>

///
/// \brief Combo box showing clipped values with an ellipsis and a full-value tooltip.
///
class ElidedComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable)

public:
    ///
    /// \brief Constructs an empty combo box.
    /// \param parent Parent widget.
    ///
    explicit ElidedComboBox(QWidget *parent = nullptr);

    ///
    /// \brief Uses an eliding line edit when manual input is enabled.
    /// \param editable Whether the user may enter a custom value.
    ///
    void setEditable(bool editable);

    ///
    /// \brief Tells whether the current value exceeds the available text width.
    /// \return True when the value needs an ellipsis.
    ///
    bool isElided() const;

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    int textWidth() const;
};
