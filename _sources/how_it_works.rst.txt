How it Works
============

An Arm primer
-------------

The Arm architecture is a RISC-style architecture,  where the instructions are mostly seemingly
simple register <-> register, and register <-> memory transfer operations.  To make those operations
performant Arm CPUs have many optimizations, two of which are particuarly interesting to us:
* pipelining
* caches

Pipelined (superscalar) implementations gain performance by splitting instructions into separate
stages (e.g. fetch -> decode -> execute) and then pushing each instruction through each stage in turn,
allowing future instructions to begin fetching/decoding and executing before the first instruction has
finished.  This gives a simple "out-of-order" appearence which is exacerbated by more direct Out-of-order
executions (e.g. re-order buffers).

However, pipelines can only do so much,  once the pipeline reaches a memory access it has to stall and wait.
To help with this, outside the pipeline is a series of fast but small caches of memory that can be used to store
and retrieve recently-accessed data much faster than going all the way out to memory.  Caches come with a litany
of consistency and ordering issues and so there exist complex cache protocols to address them.

In an ideal world both of these optimizations would be unobservable to the user (except for timing effects),
however this is not the case.  For whatever reason, CPU vendors have decided that fully hiding the details
would be too costly and so some of the out-of-order nature of the machine is observable to the user.

Historically these have been

Going up into systems
---------------------