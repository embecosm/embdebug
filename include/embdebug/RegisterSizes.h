// Register Size Information
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef REGISTERSIZES_H
#define REGISTERSIZES_H

#include <cinttypes>
#include <cstdint>

namespace EmbDebug {

#if 1
typedef uint64_t uint_reg_t;
#define PRIxREG PRIx64
#else
typedef uint32_t uint_reg_t;
#define PRIxREG PRIx32
#endif

} // namespace EmbDebug

#endif /* REGISTERSIZES_H */
