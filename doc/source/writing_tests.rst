Writing Litmus Tests
====================

Systems Harness litmus tests are defined in individual C files,
saved under a group directory in ``litmus/litmus_tests/``.

Each file contains a series of components that define the Litmus Test in
its entirety:

* a set of variables that represent chunks of virtual memory
* a set of output registers
* a C function for each thread
* a C function for each thread's synchronous exception handler, if it has one.
* a ``litmus_test_t`` declaration for the actual litmus test data.

Here is an example from ``litmus_tests/data/MP+pos.c``

.. code-block:: C

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


Variables and Registers
-----------------------

Each file contains a define macro for the set of memory locations and output registers.
These are used in multiple places to aid in the definition of the litmus test file itself.

.. code-block:: none

    #define VARS x, y
    #define REGS p1x0, p1x2

``VARS`` is a comma-separated list of memory locations, and can be named anything.
``REGS`` is a comma-separated list of output registers and must have the form pNr
where X=the thread, where 0 is the first thread and r is the register (e.g. x0-x30).

Thread Code Blocks
------------------

Each thread is defined by a single C function,  typically named P0-Pn.
This C function is executed on each loop at EL0,  and is passed a single argument
which contains all the data available to the test.

In the following snippet is an example defining Thread#3 (aka P2) and its exception handler.

.. code-block: none

    static void P2_sync_handler(void) {
        asm volatile (
            "mov x2, #0\n\t"

            ERET_TO_NEXT(x10)
        );
    }

    static void P2(litmus_test_run* data) {
        asm volatile (
            "mov x1, %[y]\n\t"
            "mov x3, %[x]\n\t"

            /* test */
            "ldr x0, [x1]\n\t"
            "eor x4,x0,x0\n\t"
            "add x4,x4,x3\n\t"
            "ldr x2, [x4]\n\t"

            /* output */
            "str x0, [%[outp2r0]]\n\t"
            "str x2, [%[outp2r2]]\n\t"
        :
        : ASM_VARS(data, VARS),
          ASM_REGS(data, REGS)
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10"
        );
    }

Register and Memory Location names
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Inside the asm block, the ``ASM_VARS`` macro makes the memory locations accessible inside the asm code,
as named identifiers in the way they were defined in the ``#define VARS`` declaration.
The ``ASM_REGS`` macro makes the name ``outpNrM`` available for register ``pNxM`` in the asm block.
Note that the set of clobbers x0-x10 are required, and the linter (see :doc:`test_discover`) should check them
if it can.

Exception handlers
^^^^^^^^^^^^^^^^^^

Here P2 has an exception handler,  there is no restriction on the names of the handler and it is called ``P2_sync_handler`` in this example.
It will be executed for any synchronous exception from the designated exception level (defined in the ``litmus_test_t`` declaration, see below).
It **must** contain an ``eret`` instruction.

This example uses the ``ERET_TO_NEXT(r)`` macro which ensures it performs an ``eret`` to the instruction after the one that caused the exception,
allowing the test to continue.  This macro clobbers register ``r`` in the process, and so it must be included in the list of clobbers in the asm block
(note how that this example includes "x10" in the clobbers).  As before, linters should check this if they can.

Litmus Test Data
----------------

The final ``litmus_test_t`` declaration contains the actual data of the litmus test that
is loaded when the tool starts.

It contains the following information:

* the litmus test name.
* the set of threads.
* the set of variables/memory locations.
* the set of output registers.
* the exception handlers for each thread.
* the initial state, which is one of the following:
    * ``INIT_VAR(x, v)`` to set the initial value of memory location ``x`` to ``v``.
    * ``INIT_UNMAPPED(x)`` to ensure the memory location ``x`` starts out unmapped and accesses to it generate translation faults.
    * ``INIT_REGION_OWN(x, r)`` ensures no vars unrelated to ``x`` will be placed into the same region of size ``r``.
    * ``INIT_REGION_PIN(x, y, r)`` ensures ``x`` is placed into the same region of size ``r`` as ``y``.
    * ``INIT_REGION_OFFSET(x, y, o)`` ensures ``x`` has the same offset into a region of size ``r`` as ``y``.
* the interesting result(s) to flag up in the final output.
* the number of non-interesting results, for sanity checking.

The actual C variable name of the ``litmus_test_t`` object is not used, but must be a unique C identifier.

An example that uses all the above features is given below, with comments:

.. code-block:: C


    litmus_test_t BBM1_dmblddsbtlbiisdmblddsbdsb_dsbisb = {

      /** the name of the litmus test as it will be used and displayed
       * by the litmus.exe tool
       */
      "MP.BBM1+[dmb.ld]-dsb-tlbiis-[dmb.ld]-dsb-dsb+dsb-isb",

      /** this test has 3 threads, and uses the VARS and REGS macros defined earlier
       */
      MAKE_THREADS(3),
      MAKE_VARS(VARS),
      MAKE_REGS(REGS),

      /** there are 5 initial state entries and they are:
       * x starts unmapped
       * y starts as 0 and is placed in its own 2M-aligned region
       * z starts as 1 and is forcibly placed in the same 2M-aligned region as y
       * a starts as 0 and is placed in the same 4k-aligned region as x
       * b starts as 0 and is placed at a location such that the VAs of b and y
       *    share the same lower 21 bits.
       *    i.e. b[20:0] == x[20:0]
       */
      INIT_STATE(
          10,
          INIT_UNMAPPED(x),
          INIT_VAR(y, 0),
          INIT_REGION_OWN(y, REGION_OWN_PMD),
          INIT_VAR(z, 1),
          INIT_REGION_PIN(z, y, REGION_SAME_PMD),
          INIT_VAR(a, 0),
          INIT_REGION_PIN(a, x, REGION_SAME_PAGE),
          INIT_VAR(b, 0),
          INIT_REGION_OFFSET(b, y, REGION_SAME_PMD_OFFSET),
      ),

      /** there are 2 interesting results
       * the case where p0x2=1, p0x7=1, p1x0=1, p1x2=0
       * and the case where p0x2=1, p0x7=1, p1x0=1, p1x2=2
       *
       * note that the registers in the result appear in the order of the #VARS declaration.
       */
      .no_interesting_results=2,
      .interesting_results = (uint64_t*[]){
          (uint64_t[]){
          /* p0:x2 =*/1,
          /* p0:x7 =*/1,
          /* p1:x0 =*/1,
          /* p1:x2 =*/0,  /* stale translation */
          },
          (uint64_t[]){
          /* p0:x2 =*/1,
          /* p0:x7 =*/1,
          /* p1:x0 =*/1,
          /* p1:x2 =*/2,  /* spurious abort */
          },
      },

      /** thread 0 starts at EL1
       * all other threads start at EL0
       */
      .start_els=(int[]){1,0,0},

      /** Threads 0 and 2 do not have any exception handlers setup
       * and so any exception will abort the test.
       *
       * Thread 1 has an exception handler for exceptions from EL0,
       * but not from EL1.  So exceptions in Thread 1 from EL1 will abort the test.
       * Exceptions in Thread 1 from EL0 will run the sync_handler function
       */
      .thread_sync_handlers =
          (uint32_t**[]){
          (uint32_t*[]){NULL, NULL},
          (uint32_t*[]){(uint32_t*)sync_handler, NULL},
          (uint32_t*[]){NULL, NULL},
          },

      /** this test requires the MMU to be enabled
       * and hence requires the --pgtable option to be passed
       */
      .requires=REQUIRES_PGTABLE,

      /** there are 14 other uninteresting results
       *
       * the harness will check that the total number of results it saw for this test was the 2
       * interesting ones, and exactly 14 distinct other results.
       *
       * if it sees fewer or more, it will emit an warning e.g.
       * e.g. "Warning on MP.BBM1+[dmb.ld]-dsb-tlbiis-[dmb.ld]-dsb-dsb+dsb-isb: saw 5 SC results but expected 14"
       */
      .no_sc_results = 14,

      /** this test's specified outcome(s) are allowed
       * in the armv8-cseh model but forbidden in the armv8 model
       *
       * this field is not read or used by the test harness
       * and is only there for external tools and documentation of the test
       */
      .expected_allowed = (arch_allow_st[]){
         {"armv8", OUTCOME_FORBIDDEN},
         {"armv8-cseh", OUTCOME_FORBIDDEN},
      },
    };