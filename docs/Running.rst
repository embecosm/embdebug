.. Copyright (C) 2019 Embecosm Limited
   SPDX-License-Identifier: CC-BY-SA-4.0

Running
-------

After building the debug server and any targets (see
:ref:`building-debug-server`), the server can be started through the
``embdebug`` executable. A typical command to run the debug server looks
as follows:

.. code-block:: none

   embdebug --soname <libtarget.so> --rsp-port <port_number>

When the debug server is started at a minimum it needs to know
two things. First it needs to know how to interact with the
target, and second it needs to know where to listen for a connection
from the debugger. In the above command ``<libtarget.so>`` is a
shared object containing an implementation of the debug server
target interface. This provides all of the behaviour required
for the server to interact with the target (for more details,
see :ref:`porting-debug-server`). Meanwhile, the ``<port_number>``
represents the port on which the debug server will listen for a
connection from the debugger. The debug server and debugger will
communicate over this port in the Remote Serial Protocol (see
:ref:`remote-serial-protocol`).

``--soname`` and ``--rsp-port`` are the most frequently used
options to the debug server, other options are documented below.

Options for embdebug
````````````````````

--rsp-port  Specify the port number to listen on, waiting for a
            connection from the debugger.  If this is not specified, and
	    ``--stdin`` is not specified, Embdebug will generate a random port
	    number in the Ephemeral range (49,152-65535).
--soname    Shared object containing an implementation of the
            target interface
--version   Print the version number of the debug server
--stdin     Instead of using a socket to communicate with the
            debug server, use ``stdin`` and ``stdout``. This
            suppresses any other output to ``stdout``.
--bufsize   Override the default size of the buffer (10,000 bytes) used for RSP
            packets.  This can be useful for very slow targets (for example
            cycle accurate simulations of JTAG interfaces to debug units) in
            order to avoid RSP timeouts.

Any other options are passed on to the target interface for it to process, so
specific targets may have further options to control their behavior.

Interacting with the debug server from GDB
``````````````````````````````````````````

There are two ways to interact with Embdebug.  The first is to start Embdebug
in a window of its own, and either specifying a port (using ``--rsp-port``) or
noting the one generated automatically.

Then start up your GDB session as normal and connect to the specified port as
a remote target. For example if Embedebug is using port 54321, you could use
the following:

.. code-block:: none

   (gdb) target remote :54321

Then just debug as normal.  When finished you can detach explictly from the
debug server using the ``detach`` command, or you can just exit GDB.

However you can also start Embdebug from within GDB, and connect to it via a
socket.  This is the purpose of the ``--stdin`` option to Embdebug.  From
within your debug session you would use

.. code-block:: none

   (gdb) target remote |embdebug --soname <libtarget.so> --stdin

(you can add any other options as well). Then just debug as normal.  The
server will terminate if you use the ``detach`` command or if you exit GDB.

Interacting with the debug server from LLDB
```````````````````````````````````````````

*To be written.*
