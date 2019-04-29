// Entry point for initializing the server
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef EMBDEBUG_INIT_H
#define EMBDEBUG_INIT_H

namespace EmbDebug {

class ITarget;
class TraceFlags;

//! \brief Initialize the GDBServer
//!
//! This continually services RSP requests, and does not return until an
//! error occurs or the GDBServer is interrupted.
//!
//! \param[in] target     Interface to the target, non-null.
//! \param[in] traceFlags Initial configuration flags for the target, non-null.
//! \param[in] useStreamConnection True if RSP traffic uses standard input and
//!                                output instream of a socket.
//! \param[in] rspPort    Port number to use for socket communication.
//! \param[in] writePort  True if the used rsp port should be written to a file.
//! \return EXIT_SUCCESS on success, or EXIT_FAILURE otherwise.
int init(ITarget *target, TraceFlags *traceFlags, bool useStreamConnection,
         int rspPort, bool writePort);

} // namespace EmbDebug

#endif
