Customizing the Build
=====================

system-litmus-harness comes with many options to customize the built binary.

To see them in full run ``make help`` to see all options available to you:

.. code-block:: none

    $ make help
    Simple Usage:
      make              equivalent to `make all`
      make all          equivalent to `make kvm qemu`
      make qemu         builds bin/qemu_litmus.exe which runs QEMU
      make kvm          builds bin/kvm_litmus.exe which uses KVM
      make run          runs `make qemu` then runs all tests
      make clean        remove built files in bin/
      make cleantests   remove auto-generated test files in litmus/

      Advanced Usage:
        [...]


Changing the compiler
---------------------

system-litmus-harness uses a set of cross-compilation tools to build
the ELF (and the eventual ``.exe`` that can be run on the target machine).

The tools the harness uses can be changed with the ``PREFIX`` argument to the makefile,
e.g. to use the ``aarch64-none-elf`` toolchain simply do:

.. code-block:: none

    $ make PREFIX="aarch64-none-elf-"


Changing the litmus tests
-------------------------

Warning: Most of the time you *do not want* to change this setting, but rather just
compile all of the tests and then pick which tests to run on the host machine.
See :doc:`customizing_litmus`.

The compiled binary contains the set of compiled litmus tests.
To change the set of litmus tests to compile use the ``LITMUS_TESTS`` argument:

.. code-block:: none

    # build the groups and test listing
    $ make litmus_tests LITMUS_TESTS=MP+pos,MP+dmbs
    # then compile them
    $ make

Tests are arranged into groups and you can supply group listings,
as well as test names,  using ``-`` s to exclude tests or test groups instead of including them:

.. code-block: none

    # builds all tests except those in the @amo group
    $ make LITMUS_TESTS=@all,-@amo


Building for KVM
----------------

The final executable passes different arguments to the final QEMU call depending on whether
it's designed for emulation or execution as a KVM guest.

Running ``make`` will build the following files:

* bin/litmus.elf        the QEMU/KVM guest ELF
* bin/litmus.bin        the stripped raw binary
* bin/kvm_litmus.exe    executable file that loads the elf and executes it with KVM
* bin/qemu_litmus.exe   executable file that loads the elf and emulates within QEMU

the executable files are identical except for the final QEMU call in each that supplies
different baked-in arguments.

.. code-block:: none

    $ make
    # ... run local with QEMU emulation
    $ ./bin/qemu_litmus.exe -h
    # shorthand:
    $ make run -- -h

    # ... or remotely
    $ scp bin/kvm_litmus.exe rpi4:litmus.exe
    $ ssh rpi4 './litmus.exe -h'