Debugging a Failing Build
=========================

system-litmus-harness uses a relatively simple Makefile to control the build process,
but it relies on many other tools, any of which could fail.

The following describes the common issues and their solutions.

AArch64 Cross-Compiler Tools
----------------------------


error: invalid instruction mnemonic
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* error: invalid instruction mnemonic 'msr'
* error: invalid instruction mnemonic 'dmb'
* error: invalid instruction mnemonic 'isb'

These errors are all caused by the makefile using the incorrect compiler.
Building the binary requires an AArch64 (Arm64) compiler, see :doc:`getting_started` to
see how to get one if you do not have one.

If you do have an Arm64 compiler e.g. ``aarch64-none-elf-gcc`` but the makefile is not detecting
it then you can tell the makefile to use that prefix directly:

.. code-block:: none

    $ make build PREFIX="aarch64-none-elf-"

error: Command 'cmd' not found!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* error: Command '\[...\]gcc' not found!
* error: Command '\[...\]ld' not found!
* error: Command '\[...\]objcopy' not found!

These errors are caused by make not being able to find one of the required cross-compilation tools.
Check that they exist (see :doc:`getting_started` to see requirements) or see previous item to
fix make's ``PREFIX``.