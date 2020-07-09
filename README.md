ARMv8-A System Litmus Runner
============================

system-litmus-harness is a small robust test harness written as an AArch64 QEMU/KVM guest.
It takes as input small C programs with some embedded A64, and runs those programs millions
of times and records the outcomes.

Each small C program ("litmus test") takes as input a set of variables with some initial state,
and outputs a set of register values.

Example from [litmus/litmus_tests/data/MP+pos.c](./litmus/litmus_tests/data/MP+pos.c):

```c
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, #1\n\t"
    "str x0, [%[x]]\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[y]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "ldr x0, [%[y]]\n\t"
    "ldr x2, [%[x]]\n\t"

    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2"
  );
}


litmus_test_t MP_pos = {
  "MP+pos",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0),
  ),
  .interesting_result =
    (uint64_t[]){
      /* p1:x0 =*/ 1,
      /* p1:x2 =*/ 0,
  },
  .no_sc_results = 3,
};
```


Build & Run
-----------

To run all the tests locally with QEMU:
```
> make run

[...]
```

But can also build a KVM guest and run it remotely:
```
> make kvm
```

The Makefile will build the ARMv8-A executable `bin/kvm_litmus.exe` which can be copied to an ARMv8-A device (such as the Raspberry Pi) with KVM installed
and ran directly.

```
> scp bin/kvm_litmus.exe pi:
> ssh pi './kvm_litmus.exe -h'
Usage: litmus.exe [OPTION]... [TEST]...
Run EL1/EL0 litmus tests

Options:
-h --help:       display this help text and quit.
-d --debug:      enable debug output.
-t --trace:      enable trace output.
-p --pgtable:    switch to pagetable tests.
-sX              initial seed (default: none)
--no-hist:       disable histogram output and print results as they are collected
-nX:             set number of runs to X (default: 10000)

Tests:
If none supplied, selects all enabled tests.
Otherwise, runs all tests supplied in args that are one of:
    MP+pos
    MP+dmbs
    [...]
```

For more information, run `make help`:

```
> make help
Simple Usage:
   make                 equivalent to `make all`
   make all             equivalent to `make kvm qemu`
   make qemu            builds bin/qemu_litmus.exe which runs QEMU
   make kvm             builds bin/kvm_litmus.exe which uses KVM
   make run             runs `make qemu` then runs all tests
   make clean           remove built files in bin/
   make cleantests              remove auto-generated test files in litmus/

Advanced Usage:
   [...]
```

Requirements
------------

To compile locally you need an `AArch64` compiler, such as `aarch64-linux-gnu-gcc`,  usually installable directly with your package manager.
This should come with `AArch64` compatible `-objdump`, `-objcopy`  and `-ld`, if not, they will need to be installed.

For debugging locally you need `gdb`.


Debugging
---------

For debugging the Makefile generates a `bin/litmus.elf.S` file.
This file contains an objdump of the binary.

Running
```
> make debug GDB="aarch64-none-elf-gdb"
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