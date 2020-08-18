Concretization
==============

A Litmus test is essentially a concrete program with symbolic inputs.
The job of the test harness is to instantiate these inputs and collect the outputs.

When to allocate
----------------

There are primarily three ways to perform the instantiation of the input data:
* ephemeral
* arrayization
* semi-arrayization

Each of these ways are similar,  they allocate regions of memory for each input variable
and then run the test.  But they differ in exactly how they would allocate those regions.


Ephemeral
^^^^^^^^^

The simplest approach is to just pick concrete addresses on each run of the test,
initialize them based on the test's initial state declaration and then run the test,
repeating the whole process however many times required (in pseudo-code):


.. code-block:: none

    for i in 1...#number-of-runs {
        input = pick-addrs(i)
        init-state(input)
        results = run-code(concrete_code, input)
        collect-results(results)
    }

Because the addresses are picked and then initialized as the tests run, different runs can overlap in memory.
This makes this the easiest to get working,  but requires much more synchronization between runs than the other approaches,
making it slow and potentially making interesting results harder to observe.

Arrayization
^^^^^^^^^^^^

A historical approach is just to allocate a dedicated piece of memory for each test,
with each input living on its own in its own location/cache-line/page or whatever is necessary for the test.

Each test is totally isolated and running the tests is trivial,  you just run the concrete program in a tight loop
passing in each tests' memory locations as inputs.  No extra synchronization is necessary  (but may still be desireable
for other reasons), e.g:


.. code-block:: none

    inputs = pick-all-addrs()
    init-all(inputs)

    for i in 1...#number-of-runs {
        input = inputs[i]
        results = run-code(concrete_code, input)
        collect-results(results)
    }


This allows the tests to run with maximal interleaving at execution time,  and maximal overlap in caches, buffers
etc

However, variables that need their own page, or directory effectively take 512x or 32,000x as much space
per address in the array.  In the worst case, a simple test with 1 variable that exists in its own page
needs 4KB of space per address and for 1M runs would require nearly 2 GB of memory allocated just for that one variable.
Variables that need 2nd level translations would need 2MB of space each, and variables that need 1st level translation isolation
would need ~1 GB per variable per run!

This is only feasible for tests whose VAs own only a small amount of memory, or for a very small number of runs.

Semi-arrayization
^^^^^^^^^^^^^^^^^

A compromise between the two would be to allocate the locations upfront,  with some overlap (e.g. allow
two instances of the same variable to exist in the same page) and to perform a quick initialization at runtime:

.. code-block:: none

    inputs = pick-all-addrs()

    for i in 1...#number-of-runs {
        input = inputs[i]
        init-state(input)
        results = run-code(concrete_code, input)
        collect-results(results)
    }


... TODO: is this good or bad ?

Picking the addresses
---------------------

No matter which approach is used, concrete addresses must be picked from the pool of available addresses.
But the addresses cannot simply be randomly picked,  there are constraints each must abide.

Constraints
^^^^^^^^^^^

Take the following concrete initial state,  for a test with 3 variables ``x``, ``y`` and ``z``.
The test writes to ``x``,  writes to the last-level translation table entry of ``y`` and to the
2nd-level translation table entry of ``z``.  This imposes the following constraints on the variables:
* ``x`` must be 64-bit aligned.
* ``y``'s 4k-aligned region (page) must not contain ``x`` or ``z``, and ``y`` must be 64-bit aligned.
* ``z``'s 2M-aligned region (dir) must not contain ``x`` or ``y``, and ``z`` must be 64-bit aligned.

Let ``x[i]`` notate the address of ``x`` but on run ``i``.

* for arrayization, forall distinct ``i``, ``i'``:

    * ``y[i]``'s page must not contain ``x[i']``, ``y[i']`` or ``z[i']``.
    * ``z[i]``'s dir must not contain ``x[i']``, ``y[i']`` or ``z[i']``.


Coverage
^^^^^^^^

The exact relationship between the variables' virtual addresses may effect how likely
certain relaxed outcomes may be.  For example,  some outcome might be more likely if two variables share the same last 12 bits.
(aka they have the same physical offset in a page).  Whereas other outcomes may become more likely if the two variables are "close"
together (in the same cache-line, page, TLB line etc).

A single run of a test can only test one of these shapes, but over many runs it should be possible to try out many combinations of
different layouts of the variables to maximally provoke the CPU to give interesting results.

But it is not clear exactly *what* the possible interesting shapes are,  or indeed *how much* they effect the outcome.