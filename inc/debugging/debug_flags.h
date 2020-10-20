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

#endif /* DEBUG_FLAGS_H */