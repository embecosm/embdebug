# Embecosm Debug Server

This contains the source for the Embecosm Debug Server which can be
used alongside GDB or LLDB to debug bare-metal targets.

The Embecosm Debug Server is open source and available under the
GPLv3+ license, with an exception for the target interface, which is available
under the MIT license. The terms of the license can be found in `LICENSE`.

# Minimum requirements

* `MSVC 2015`, `Clang 3.4`, or `GCC 5`
* `cmake 3.4.3`

# Building the debug server without a target

_**NOTE**: These are basic steps to build the debug server however this **does
not** include a target for the server, which in this case would need to be
built separately.  See below for building the server and the target in a
single step.

A more complete guide with an example target (ri5cy for a 32-bit RISC-V
system) can be found in:_ `docs/Ri5cyTutorial`

The `cmake` build system is used to build the project. Running
the follow steps from the build directory are sufficient to
build and install the debug server:

```
cmake -DCMAKE_INSTALL_PREFIX=<install_dir> <source_dir>
cmake --build .
cmake --build . --target install
```

The result of the build is the `embdebug` program, which is
used to start a server which GDB or LLDB can connect to.

## Build options

This is a quick reference for options which can be provided
to `cmake`:

Flag                            | Description
------------------------------- | -------------------------
`-DCMAKE_INSTALL_PREFIX`        | Directory the debug server will be installed to.
`-DEMBDEBUG_ENABLE_WERROR`      | Enable `-Werror` (or equivalent) when building
`-DEMBDEBUG_ENABLE_DOXYGEN`     | Enable generation of API docs using `doxygen`.
`-DEMBDEBUG_ENABLE_DOCS`        | Enable generation of project documentation using `sphinx`
`-DEMBDEBUG_TARGETS_TO_BUILD`   | Comma separated list of directories in `targets/` to be built alongside the debug server.

For more complete build options, see `docs/Building`

# Building the debug server with a target

To build the target as well as the server, you just need to link the source
tree of the target into the `targets` directory of Embdebug. The target should
use Cmake for configuration, and have its own CMakeLists.txt at the top level.

Build and configure is then identical to the steps for the debug server
without a target.  The only change is that you can also add build options
specific to the target to the initial cmake command.

# Running

The debug server can be started with the following command:

```
embdebug --soname <libtarget.so> --rsp-port <port_number>
```

This will expose a debug server on the provided port which
can be connected to from GDB or LLDB.

For more detail about options to the `embdebug` command, see
`docs/Running`

## Connecting to a local debug server

To connect with the debug server, first it needs to be running
locally:

```
embdebug --soname <libtarget.so> --rsp-port <port_number>
```

Now the debug server is waiting for a connection from a debugger.
Attaching from the debugger depends on the debugger being used.

### Connecting from GDB

```
(gdb) target remote :<port_number>
```

or using the pipe interface
```
(gdb) target remote |embdebug -s <soname>
```
(you can add any other embdebug command line options).

### Connecting from LLDB

```
(lldb) gdb-remote <port_number>
```

## Run options

A quick reference for options which can be provided to
`embdebug`:

Flag                         | Description
---------------------------- | -----------
`--rsp-port <portnum>`       | Port number that the debugger will connect to.
`--soname <shared_object>`   | Shared object containing a target exposing the target interface.
`--version`                  | Print the version number of the debug server.
`--stdin`                    | Communicate with the debugger over standard input/output instead of over a socket.

# Testing

The debug server contains a test suite which can be executed
using `gtest`:

```
cmake --build . --target test
```
