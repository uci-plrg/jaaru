/** @file snapshot.h
 *	@brief Snapshotting interface header file.
 */

#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H

#include "snapshot-interface.h"
#include "config.h"
#include "mymemory.h"

#define SHARED_MEMORY_DEFAULT  (400 * ((size_t)1 << 20))
mspace create_shared_mspace();
static struct fork_snapshotter *fork_snap = NULL;
#endif
