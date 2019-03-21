// Entry point for initializing the server
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef MAIN2_H
#define MAIN2_H

#include "ITarget.h"
#include "TraceFlags.h"

namespace EmbDebug {

int main2(ITarget *target, TraceFlags *traceFlags, bool useStreamConnection,
          int rspPort, bool writePort);

} // namespace EmbDebug

#endif
