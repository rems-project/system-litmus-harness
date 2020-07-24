Running the Tests
=================

A built executable containing litmus tests can be copied over the target
device and executed.  This executable accepts arguments

Usage
-----

Usage: ``litmus.exe [OPTION]... [TEST]...``

.. code-block:: none

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


Test Groups
-----------

The compiled tests are arranged hierarchically into *groups*.
Each group consists of a set of groups and a set of tests.

The tests are found in ``litmus/litmus_tests/``,
here is an example layout:

.. code-block:: none

    + all
    |   + data
    |   |   | MP+pos.c
    |   |   | MP+dmbs.c
    |   |   | SB+dmbs.c
    |   + exc
    |   |   | MP+dmb+svc.c
    |   + pgtable
    |   |   | CoRT.c


Here the group @all means all the tests.
The group @data contains only the 3 tests under ``data/``.

Selection of the tests can be passed to the executable:

.. code-block:: none

    # run all the tests
    $ ./litmus.exe
    # or ...
    $ ./litmus.exe @all

    # run two tests
    $ ./litmus.exe MP+pos MP+dmbs

    # run the entire data group
    $ ./litmus.exe @data


To see the groups that exist in the compiled binary run ``./litmus.exe --show``:

.. code-block:: none

    $ ./litmus.exe --show
    Tests:
    If none supplied, selects all enabled tests.
    Otherwise, runs all tests supplied in args that are one of:
    @all:
    @data:
     LB+pos
     MP+dmbs
     MP+pos
     [...]
    @exc:
     LB+R-svc-W+R-svc-W
     MP+W-svc-W+addr
     MP+dmb+R-svc-eret-R
     MP+dmb+eret
     [...]
    @pgtable:
     MP.BBM1+[dmb.ld]-dsb-tlbiis-[dmb.ld]-dsb-dsb+dsb-isb (requires --pgtable)
     MP.BBM1+dsb-isb-tlbiis-dsb-isb-dsb-isb+dsb-isb (requires --pgtable)
     MP.BBM1+dsb-tlbiis-dsb-dsb+dsb (requires --pgtable)
     [...]

(``[...]`` signifies that part of the output has been truncated for display here)

Customizing the runs
--------------------

As described in :doc:`getting_started` the tool runs the tests
in a loop.  The exact number of runs can be customized with the ``-n`` flag.

.. code-block:: none

    # run test MP+pos 500k times
    $ ./litmus.exe MP+pos -n500_000

    # run all tests 10k times
    $ ./litmus.exe @all -n10_000


Enabling translation table tests
--------------------------------

By default the MMU is switched off during the tests, and
attempting to run any pagetable tests will fail.

The MMU can enabled with the ``-p`` (or ``--pgtable``) flag.

.. code-block:: none

    # run the CoWT pagetable test 10k times
    $ ./litmus.exe CoWT -p -n10_000