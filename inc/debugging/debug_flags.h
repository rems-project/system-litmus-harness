#ifndef DEBUG_FLAGS_H
#define DEBUG_FLAGS_H

/* the --debug flag prints out a debug message
 * whenever debug() is called.
 *
 * by default this is only for a small number of special events
 *
 * this file has #defines for compile-time directives for enabling/disabling
 * debugging info for other runtime events too
 *
 * e.g. for info about allocations, and synchronization
 * that are too verbose for usual --debug output.
 */

/**
 * if -DDEBUG_BWAITS=1 then --debug will
 * print out BWAIT() calls
 */
#ifndef DEBUG_BWAITS
#define DEBUG_BWAITS 0
#endif

/**
 * if -DDEBUG_ALLOCS=1 then --debug
 * will print out ALLOC_MANY() and ALLOC_ONE() calls
 *
 * this is enabled by default
 */
#ifndef DEBUG_ALLOCS
#define DEBUG_ALLOCS 1
#endif

/**
 * if -DDEBUG_CONCRETIZATION=1 then --debug
 * will output information during concretization process
 * for debugging errors during concretization
 */
#ifndef DEBUG_CONCRETIZATION
#define DEBUG_CONCRETIZATION 0
#endif

/**
 * if -DDEBUG_LOCKS=1 then --debug
 * will output information when taking a lock
 */
#ifndef DEBUG_LOCKS
#define DEBUG_LOCKS 0
#endif

/**
 * if -DDEBUG_DISABLE_WFE=1 then nop wfe/sev
 */
#ifndef DEBUG_DISABLE_WFE
#define DEBUG_DISABLE_WFE 0
#endif

/**
 * if -DDEBUG_PTABLE=1 then
 * pagetable range set functions will
 * dump more information
 */
#ifndef DEBUG_PTABLE
#define DEBUG_PTABLE 0
#endif

/**
 * if -DDEBUG_PTABLE_DUMP_TABLES=1 then
 * after allocating a new pagetable, dump the table to the serial bus
 * in a human-readable format
 */
#ifndef DEBUG_PTABLE_DUMP_TABLES
#define DEBUG_PTABLE_DUMP_TABLES 0
#endif

/**
 * if -DDEBUG_TRACE_RUN_LOOP=1 then
 * each step of the main run loop will be traced
 */
#ifndef DEBUG_TRACE_RUN_LOOP
#define DEBUG_TRACE_RUN_LOOP 0
#endif

/**
 * if -DDEBUG_ALLOC_META=1 then
 * each ALLOC() stores some metadata about time/place of allocation
 */
#ifndef DEBUG_ALLOC_META
#define DEBUG_ALLOC_META 0
#endif

/**
 * if -DDEBUG_TRACE_VMM_ENSURES=1 then
 * each vmm_ensure_level() traces each access
 * (Warning: lots of output!)
 */
#ifndef DEBUG_TRACE_VMM_ENSURES
#define DEBUG_TRACE_VMM_ENSURES 0
#endif

/**
 * if -DDEBUG_TRACE_BOOT=1 then
 * trace through the initial boot sequence in detail
 */
#ifndef DEBUG_TRACE_BOOT
#define DEBUG_TRACE_BOOT 1
#endif

#endif /* DEBUG_FLAGS_H */