Getting Started
===============

    Traditionally, a litmus test is a small parallel program designed
    to exercise the memory model of a parallel, shared-memory, computer. [litmus7]_

How it works
------------

Systems Litmus Harness (system-litmus-harness) is a tool that allows the user to
provide small programs that run inside a kernel, that the tool boots in a hypervisor
many times over.

Building system-litmus-harness generates a small kernel that is intended to be run inside KVM [kvm]_.
When the kernel boots it loads a set of pre-determined *litmus tests* and runs each a number of times
before moving onto the next.

Each litmus test is an individual C file (located in ``system-litmus-harness/litmus/litmus_tests``) which
are discovered by the build system and automatically loaded into the binary.  The litmus tests are comprised
of a few components:

* a set of heap variables
* a set of output registers
* blocks of code for each thread
* blocks of code for each thread's synchronous exception handler (optional)
* the test declaration (name, initial values, interesting outcomes etc)

The basic flow is that the harness will initialise the test state with the initial values,
load the exception handlers into the vector tables then start each threads' code running
at approximately the same moment.  Once all threads have finished it collects all the output
registers and saves the result,  before either running the test again, or if finished printing the results.

Requirements
------------

Compiler Toolchain
^^^^^^^^^^^^^^^^^^

To cross-compile the ELF binary system-litmus-harness requires the ARMv8-A 64-bit cross-compilation
toolchain.

The required tools can be downloaded with your systems package manager:
search for packages that provide ``aarch64-linux-gnu-gcc`` or ``aarch64-none-elf-gcc``.

Or it can downloaded from Arm's webpage [armv8-tools]_.


QEMU/KVM
--------

To run the final executables the target machine must contain a version of QEMU installed.
This includes the local machine if the user wishes to run the tool locally (e.g. for debugging purposes)
under QEMU emulation.

QEMU can be downloaded from your systems package manager, or can be downloaded manually [qemu]_.

For native execution on the remote machine, the target must be running a version of Linux compiled with KVM [kvm]_.

You can check whether your device supports KVM by seeing if the ``/dev/kvm`` file exists,
e.g. checking if the device named 'rpi4' supports KVM via SSH:

.. code-block:: none

    $ ssh rpi4
    pi@rems-rpi4b:~$ ls -l /dev/kvm
    crw-rw---- 1 root kvm 10, 232 Jul 22 21:22 /dev/kvm

Debugging
^^^^^^^^^

The final binary can be debugged using GDB,  which can attach to the guest O/S.
To do this the target machine must have a version of GDB installed, or if debugging
the local emulation a multiarch or aarch64 version of GDB must be installed.

GDB can be installed from your local package manager [gdb]_.

For more information about debugging the binary, see :doc:`debugging_code`.

Installation and building
-------------------------

The source code can be pulled from GitHub:

.. code-block:: none

  $ git pull https://github.com/rems-project/system-litmus-harness.git
  $ cd system-litmus-harness

If you want to just build with defaults you can just run the makefile:

.. code-block:: none

  $ make

If that does not work, see :doc:`debugging_make`.

For more customization of the built binary see :doc:`customizing_make`.

.. _getting_started_running:

Running the tool
----------------

Once the binary has been built there are primarily two modes of operation,
the build script generates two executables:

* ``./qemu_litmus`` for local QEMU emulation
* ``./kvm_litmus`` for remote KVM execution

To run the local version simply build and run the qemu executable:

.. code-block:: none

    $ make build
    $ ./qemu_litmus MP+pos
    Test MP+pos:
     p1:x0=1  p1:x2=1  : 1240
     p1:x0=0  p1:x2=0  : 1395
     p1:x0=0  p1:x2=1  : 7365
    Observation MP+pos: 0 (of 10000)

To run it on the remote device (e.g. via SSH to a Raspberry Pi 4B+):

.. code-block:: none

    $ make
    $ scp kvm_litmus rpi4b:litmus.exe
    $ ssh rpi4b './litmus.exe MP+pos'
    Test MP+pos:
     p1:x0=1  p1:x2=1  : 2346
     p1:x0=0  p1:x2=0  : 3712
     p1:x0=0  p1:x2=1  : 2268
     p1:x0=1  p1:x2=0  : 1674 *
    Observation MP+pos: 1674 (of 10000)

For more information about the options and running litmus tests see :doc:`customizing_litmus`.


References
----------

.. [litmus7] http://diy.inria.fr/doc/litmus.html
.. [armv8-tools] https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads
.. [qemu] https://www.qemu.org/download/
.. [kvm] https://www.linux-kvm.org/page/Main_Page
.. [gdb] https://www.gnu.org/software/gdb/