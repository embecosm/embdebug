.. Copyright (C) 2019 Embecosm Limited
   SPDX-License-Identifier: CC-BY-SA-4.0

Debug Server Internals
----------------------

*To be written.*

Project Structure
`````````````````

+--------------------------+--------------------------------------------------------+
| Directory                | Description                                            |
+==========================+========================================================+
| ``server/``              | Implementation of the core of the debug server. These  |
|                          | files are built into the ``libembdebug`` library.      |
+--------------------------+--------------------------------------------------------+
| ``targets/``             | Targets to be built alongside the debug server. Each   |
|                          | target will result in a shared object which can be     |
|                          | used as a target for the debug server.                 |
+--------------------------+--------------------------------------------------------+
| ``targets/emptytarget/`` | Template target with stubbed out implementations.      |
+--------------------------+--------------------------------------------------------+
| ``tools/``               | Tools which will be used with the debug server library |
+--------------------------+--------------------------------------------------------+
| ``tools/driver/``        | Source for the ``embdebug`` driver.                    |
+--------------------------+--------------------------------------------------------+
| ``include/embdebug/``    | Headers installed as part of the project. This defines |
|                          | the external interface of ``libembdebug`` as well      |
|                          | as the interface to be implemented by targets.         |
+--------------------------+--------------------------------------------------------+
| ``vendor/``              | External projects used by the debug server.            |
+--------------------------+--------------------------------------------------------+
| ``docs/``                | Project documentation.                                 |
+--------------------------+--------------------------------------------------------+
| ``test/``                | Internal test suite.                                   |
+--------------------------+--------------------------------------------------------+
| ``cmake/``               | Common utilities used by various ``CMakeLists.txt`` in |
|                          | the project.                                           |
+--------------------------+--------------------------------------------------------+
| ``.jenkins/``            | Files used by Jenkins for automated testing.           |
+--------------------------+--------------------------------------------------------+

Doxygen
```````

The API for the debug server is annotated with Doxygen comments. These can
be generated using the ``-DEMBDEBUG_ENABLE_DOXYGEN`` option when configuring
with ``cmake``.

Build System
````````````

*To be written.*

Targets
```````

*To be written.*

.. _internals-test-suite:

Testsuite
`````````

*To be written.*
