Fixing Runtime Errors
=====================

No GICv2
--------

If the GIC (Arm Generic Interrupt Controller) is not supported by the device
then QEMU may either not terminate or try emulate it generating a warning.


warning: gic-version=host not relevant
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* qemu-system-aarch64: warning: gic-version=host not relevant with kernel-irqchip=off as only userspace GICv2 is supported. Using v2 ...

Use ``HOST=no-gic`` as an option to the makefile

.. code-block:: none

    $ make kvm HOST=no-gic

Using GDB
---------

As described in the :doc:`getting_started` section,  GDB can be used to
debug the guest binary.  GDB can either be used to debug the local emulated
binary, or the remote one running in KVM:

Debugging the local emulated version:

.. code-block:: none

    $ make debug-litmus
    [...]

Or on the target machine

.. code-block:: none

    $ ssh rpi4
    pi@rems-rpi4b:~$ { QEMU_ARGS='-s -S' ./litmus.exe & } &
    pi@rems-rpi4b:~$ gdb --eval-command "target remote :1234"
    [...]