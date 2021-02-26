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


Quick Start
------------

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
-h --help:                 display this help text and quit.
-nX:                       set number of runs to X (default: 10000)
-d --debug:                enable debug output.
-t --trace:                enable trace output.
--verbose / --no-verbose:  enabled/disable verbose output.
--pgtable / --no-pgtable:  enabled/disable pagetable tests.
--no-hist:                 disable histogram output and print results as they are collected
--show:                    display the list of tests and quit
-sX                        initial seed (default: none)

> ssh pi './kvm_litmus.exe MP+pos'
Test MP+pos:
 p1:x0=1  p1:x2=1  : 2346
 p1:x0=0  p1:x2=0  : 3712
 p1:x0=0  p1:x2=1  : 2268
 p1:x0=1  p1:x2=0  : 1674 *
Observation MP+pos: 1674 (of 10000)
```

For more details, see the [documentation](https://rems-project.github.io/system-litmus-harness/).


Funding
-------

This software was developed by the University of Cambridge Computer
Laboratory (Department of Computer Science and Technology), in part
funded by an Arm iCASE doctoral studentship (18000005, Simner) in part
funded from the European Research Council (ERC) under the European
Union’s Horizon 2020 research and innovation programme (grant
agreement No 789108, "ELVER"), in part funded by Arm, and in part
funded by Google.
