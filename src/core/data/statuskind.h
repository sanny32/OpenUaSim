// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file statuskind.h
/// \brief Declares the semantic state shown by status indicators.
///

#pragma once

///
/// \brief Semantic state that views render as a coloured indicator.
///
enum class StatusKind {
    None,
    Good,
    Info,
    Warning,
    Error,
    Inactive
};
