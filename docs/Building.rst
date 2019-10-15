.. Copyright (C) 2019 Embecosm Limited
   SPDX-License-Identifier: CC-BY-SA-4.0

.. _building-debug-server:

Building the Debug Server
-------------------------

The Embecosm Debug Server uses CMake for its build system.

Minimal build steps
```````````````````

The basic steps to configure and build the debug server, plus any targets
discovered in the ``target/`` subdirectory of the project.

.. code-block:: none

   cmake -DCMAKE_INSTALL_PREFIX=<install_dir> <source_dir>
   cmake --build .
   cmake --build . --target install

CMake build options
```````````````````

-DCMAKE_INSTALL_PREFIX       The directory that the debug server will be
                             installed to
-DEMBDEBUG_ENABLE_WERROR     Enable ``-Werror`` (or equivalent) when building
                             the debug server)
-DEMBDEBUG_ENABLE_DOXYGEN    Enable the building of API documentation using
                             Doxygen. This requires that ``doxygen`` is
                             is available on the path.
-DEMBDEBUG_ENABLE_DOCS       Enable the build of this Sphinx-generated
                             documentation. This requires that ``sphinx-build``
                             is available on the path.
-DEMBDEBUG_TARGETS_TO_BUILD  Comma separate string for each of the targets
                             in the ``target/`` subdirectory to be configured
                             and built alongside the debug server. This
                             defaults to building all discovered targets.

Testing
```````

The debug server has a test suite using Google Test which can
be run as part of the build. To run the tests use the following
CMake command:

.. code-block:: none

   cmake --build . --target test

For details on the structure of the test suite, see
:ref:`internals-test-suite`.
