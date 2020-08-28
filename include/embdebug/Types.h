// Register and Address Type Information
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2017-2019 Embecosm Limited
// SPDX-License-Identifier: MIT
// ----------------------------------------------------------------------------

#ifndef EMBDEBUG_GDBSERVER_TYPES_H
#define EMBDEBUG_GDBSERVER_TYPES_H

#include <cinttypes>
#include <cstdint>

namespace EmbDebug {

typedef uint64_t uint_reg_t;
#define PRIxREG PRIx64
typedef uint64_t uint_addr_t;
#define PRIxADDR PRIx64

} // namespace EmbDebug

#endif
