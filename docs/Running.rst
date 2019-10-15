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
            connection from the debugger
--soname    Shared object containing an implementation of the
            target interface
--version   Print the version number of the debug server
--stdin     Instead of using a socket to communicate with the
            debug server, use ``stdin`` and ``stdout``. This
            suppresses any other output to ``stdout``.

Interacting with the debug server from GDB
``````````````````````````````````````````

*To be written.*

Interacting with the debug server from LLDB
```````````````````````````````````````````

*To be written.*
