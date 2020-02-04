ARMv8-A System Litmus Runner
============================


Build & Run
-----------

Run
```
> make
> make run
```

The Makefile will build the ARMv8-A executable `main.bin` which can be copied to an ARMv8-A device (such as the Raspberry Pi) with KVM installed,  or can be ran locally under QEMU emulation.

Requirements
------------

To compile locally you need an `AArch64` compiler, such as `aarch64-linux-gnu-gcc`,  usually installable directly with your package manager.

For debugging you need `gdb-multiarch`,  again you should be able to just install it with your package manager.


Debugging
---------

For debugging the Makefile generates a `main.elf.s` file.
This file contains an objdump of the binary.

Running
```
> make debug
```

will start a local session in QEMU and attach gdb to it,  you can then navigate around in the usual way (example session below):

    GNU gdb (Ubuntu 8.2.91.20190405-0ubuntu3) 8.2.91.20190405-git
    Copyright (C) 2019 Free Software Foundation, Inc.
    License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses     gpl.html>
    This is free software: you are free to change and redistribute it.
    There is NO WARRANTY, to the extent permitted by law.
    Type "show copying" and "show warranty" for details.
    This GDB was configured as "x86_64-linux-gnu".
    Type "show configuration" for configuration details.
    For bug reporting instructions, please see:
    <http://www.gnu.org/software/gdb/bugs/>.
    Find the GDB manual and other documentation resources online at:
        <http://www.gnu.org/software/gdb/documentation/>.

    For help, type "help".
    Type "apropos word" to search for commands related to "word".
    The target architecture is assumed to be aarch64
    Remote debugging using localhost:1234
    warning: No executable has been specified and target does not   support
    determining executable automatically.  Try using the "file" command.
    0x0000000040000000 in ?? ()
    (gdb) break *0x40080000   ; break at <start>
    Breakpoint 1 at 0x40080000
    (gdb) c
    Continuing.

    Thread 1 hit Breakpoint 1, 0x0000000040080000 in ?? ()
    (gdb) x/i $pc    ; print instruction
    => 0x40080000:  adrp    x9, 0x40080000
    (gdb) stepi      ; step 1 instruction
    0x0000000040080004 in ?? ()
    (gdb) stepi
    0x0000000040080008 in ?? ()
    (gdb) x/i $pc
    => 0x40080008:  adrp    x10, 0x40090000
    (gdb) x $x9      ; print value of x9  (the base load address)
       0x40080000:  adrp    x9, 0x40080000
    (gdb) watch *0x400965f8   ; add watchpoint on <vmm_pgt_idmap>
    Hardware watchpoint 2: *0x400965f8
    (gdb) c
    Continuing.

    Thread 1 hit Hardware watchpoint 2: *0x400965f8

    Old value = 0
    New value = 1207955456
    0x0000000040080148 in ?? ()   ; PC = in <setup> after str to    vmm_pgt_idmap

Note: QEMU loads the binary to location `0x40080000` by default. Exact addresses will vary.


How it works
------------

TODO.