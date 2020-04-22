/** @file config.h
 * @brief Configuration file.
 */

#ifndef CONFIG_H
#define CONFIG_H

/** Turn on debugging. */
#ifndef CONFIG_DEBUG
 #define CONFIG_DEBUG
 #endif

 #ifndef CONFIG_ASSERT
 #define CONFIG_ASSERT
 #endif

/** Snapshotting configurables */

/** Size of signal stack */
#define SIGSTACKSIZE 65536

/** Page size configuration */
#define PAGESIZE 4096

/* Size of stack to allocate for a thread. */
#define STACK_SIZE (1024 * 1024)

/** Enable debugging assertions (via ASSERT()) */
#define CONFIG_ASSERT


#endif
