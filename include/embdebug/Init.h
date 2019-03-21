// Entry point for initializing the server
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef INIT_H
#define INIT_H

namespace EmbDebug {

class ITarget;
class TraceFlags;

int init(ITarget *target, TraceFlags *traceFlags, bool useStreamConnection,
         int rspPort, bool writePort);

} // namespace EmbDebug

#endif
