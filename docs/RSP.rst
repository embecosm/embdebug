.. Copyright (C) 2019 Embecosm Limited
   SPDX-License-Identifier: CC-BY-SA-4.0

.. _remote-serial-protocol:

The Remote Serial Protocol (RSP)
--------------------------------

The Remote Serial Protocol (referred to simply as *RSP* going forward) is the
primary interface used by the debugger (GDB or LLDB) to control a remote
target. The protocol is text based, and typically used over a serial
connection, TCP socket or simply over standard input/output. A more
comprehensive description of the protocol can be found in 
`Embecosm Application Note 4
<https://www.embecosm.com/resources/appnotes/#EAN4>`_,
however a basic description of the protocol is below.

Basic Operation
```````````````

The two ends of the RSP communication are the debugger (the client) and
the GDBServer (the server). All communication from the server is in
response to a request from the client.

Before the client can attach to the debug server, the debug server needs to be
running and listening on a port. For the Embecosm Debug Server this is achieved
through a command such as:

.. code-block:: none

   embdebug --soname <libtarget.so> --rsp-port 5544

Once the debug server is running, we can connect the client (GDB for this
example) to it with a command such as follows:

.. code-block:: none

   (gdb) target remote :55444``

Now, any user commands to the debugger will result in RSP communication
between the debugger and the GDBServer. As a user we can observe this
raw communication through the following (GDB specific) command:

.. code-block:: none

   (gdb) set debug remote 1``

User commands to the debugger will often result in a lot of packets
flying backwards and forwards, however whilst there is much chatter
behind the scenes the protocol itself is made up of very few distinct packets.

RSP packets
```````````

Packets all start with a ``$``, followed by a payload, then a ``#``, then
a one byte checksum represented by two ASCII-encoded hexadecimal digits. The
exact contents of the payload depend on the type of the packet, which is
defined by the character(s) immediately after the ``$``. For example the
following are valid RSP packets:

.. code-block:: none

   $m10074,2#c7``
   $g#67``

A minimal implementation on the server of RSP needs to implement only the
packets starting with the following prefixes:

+-----------------+-------------------------------------------------------+
| Packet prefix   | Description                                           |
+=================+=======================================================+
| ``$?``          | Report the reason that the target halted              |
+-----------------+-------------------------------------------------------+
| ``$c``, ``$C``  | Continue execution                                    |
+-----------------+-------------------------------------------------------+
| ``$s``, ``$S``  | Step a single instruction                             |
+-----------------+-------------------------------------------------------+
| ``$D``          | Detach from the client                                |
+-----------------+-------------------------------------------------------+
| ``$g``          | Read all general purpose registers                    |
+-----------------+-------------------------------------------------------+
| ``$G``          | Write all general purpose registers                   |
+-----------------+-------------------------------------------------------+
| ``$qC``         | Report the current thread                             |
+-----------------+-------------------------------------------------------+
| ``$qH``         | Set the current thread for subsequence operations     |
+-----------------+-------------------------------------------------------+
| ``$k``          | Kill the target                                       |
+-----------------+-------------------------------------------------------+
| ``$m``          | Read memory                                           |
+-----------------+-------------------------------------------------------+
| ``$M``          | Write memory                                          |
+-----------------+-------------------------------------------------------+
| ``$p``          | Read a specific register                              |
+-----------------+-------------------------------------------------------+
| ``$P``          | Write a specific register                             |
+-----------------+-------------------------------------------------------+
| ``$qOffsets``   | Report offsets used when relocated downloaded code    |
+-----------------+-------------------------------------------------------+
| ``$qSupported`` | Report features supported by the server               |
+-----------------+-------------------------------------------------------+
| ``$qSymbol``    | Request any symbol table data                         |
+-----------------+-------------------------------------------------------+
| ``$vCont?``     | Report the type of `vCont` actions that are supported |
+-----------------+-------------------------------------------------------+
| ``$X``          | Write binary data to the target                       |
+-----------------+-------------------------------------------------------+
| ``$z``          | Clear a breakpoint at an address                      |
+-----------------+-------------------------------------------------------+
| ``$Z``          | Set a breakpoint at an address                        |
+-----------------+-------------------------------------------------------+
