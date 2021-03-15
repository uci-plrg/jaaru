/** @file datarace.h
 *  @brief Data race detection code.
 */

#ifndef __DATARACE_H__
#define __DATARACE_H__

#include "config.h"
#include <stdint.h>
#include "modeltypes.h"
#include "classlist.h"
#include "hashset.h"
#define SHADOWTABLESIZE 65536

struct ShadowTable {
	void * array[SHADOWTABLESIZE];
};

struct ShadowBaseTable {
	uint64_t array[SHADOWTABLESIZE];
	uint8_t data[SHADOWTABLESIZE];
};

struct DataRace {
	/* Clock and thread associated with first action.  This won't change in
	         response to synchronization. */

	thread_id_t oldthread;
	modelclock_t oldclock;
	/* Record whether this is a write, so we can tell the user. */
	bool isoldwrite;

	/* Model action associated with second action.  This could change as
	         a result of synchronization. */
	ModelAction *newaction;
	/* Record whether this is a write, so we can tell the user. */
	bool isnewwrite;

	/* Address of data race. */
	const void *address;
	void * backtrace[64];
	int numframes;
};

uint8_t * lookupShadowEntry(const void *address);

#define MASK16BIT 0xffff
void initRaceDetector();
void resetRaceDetector(bool resetData = false);
void raceCheckWrite(thread_id_t thread, void *location);
void atomraceCheckWrite(thread_id_t thread, void *location);
void raceCheckRead(thread_id_t thread, const void *location);
void atomraceCheckRead(thread_id_t thread, const void *location);
void recordWrite(thread_id_t thread, void *location);
void recordCalloc(void *location, size_t size);
void assert_race(struct DataRace *race);
bool hasNonAtomicStore(const void *location);
void setAtomicStoreFlag(const void *location);
void getStoreThreadAndClock(const void *address, thread_id_t * thread, modelclock_t * clock);

void raceCheckRead8(thread_id_t thread, const void *location);
void raceCheckRead16(thread_id_t thread, const void *location);
void raceCheckRead32(thread_id_t thread, const void *location);
void raceCheckRead64(thread_id_t thread, const void *location);

void raceCheckWrite8(thread_id_t thread, const void *location);
void raceCheckWrite16(thread_id_t thread, const void *location);
void raceCheckWrite32(thread_id_t thread, const void *location);
void raceCheckWrite64(thread_id_t thread, const void *location);


inline bool ValidateAddress8(void * address) {
	volatile uint8_t * val1 = ((volatile uint8_t *)(lookupShadowEntry(address)));
	uint8_t val2 = *((volatile uint8_t *)address);
	bool val = (*val1) != val2;
	if (val)
		*val1 = val2;
	return val;
}

//Address must be 8 byte aligned
inline bool ValidateAddress64(void * address) {
	ASSERT((((uintptr_t)address)&0x7)==0);
	volatile uint64_t * val1 = ((volatile uint64_t *)(lookupShadowEntry(address)));
	uint64_t val2 = *((volatile uint64_t *)address);
	bool val = (*val1) != val2;
	if (val)
		*val1 = val2;
	return val;
}

/**
 * @brief A record of information for detecting data races
 */
struct RaceRecord {
	modelclock_t *readClock;
	thread_id_t *thread;
	int numReads : 31;
	int isAtomic : 1;
	thread_id_t writeThread;
	modelclock_t writeClock;
};

unsigned int race_hash(struct DataRace *race);
bool race_equals(struct DataRace *r1, struct DataRace *r2);

#define INITCAPACITY 4

#define ISSHORTRECORD(x) ((x)&0x1)

#define THREADMASK 0x3f
#define RDTHREADID(x) (((x)>>1)&THREADMASK)
#define READMASK 0x1ffffff
#define READVECTOR(x) (((x)>>7)&READMASK)

#define WRTHREADID(x) (((x)>>32)&THREADMASK)

#define WRITEMASK READMASK
#define WRITEVECTOR(x) (((x)>>38)&WRITEMASK)

#define ATOMICMASK (0x1ULL << 63)
#define NONATOMICMASK ~(0x1ULL << 63)

/**
 * The basic encoding idea is that (void *) either:
 *  -# points to a full record (RaceRecord) or
 *  -# encodes the information in a 64 bit word. Encoding is as
 *     follows:
 *     - lowest bit set to 1
 *     - next 6 bits are read thread id
 *     - next 25 bits are read clock vector
 *     - next 6 bits are write thread id
 *     - next 25 bits are write clock vector
 *     - highest bit is 1 if the write is from an atomic
 */
#define ENCODEOP(rdthread, rdtime, wrthread, wrtime) (0x1ULL | ((rdthread)<<1) | ((rdtime) << 7) | (((uint64_t)wrthread)<<32) | (((uint64_t)wrtime)<<38))

#define MAXTHREADID (THREADMASK-1)
#define MAXREADVECTOR (READMASK-1)
#define MAXWRITEVECTOR (WRITEMASK-1)

#define INVALIDSHADOWVAL 0x2ULL
#define CHECKBOUNDARY(location, bits) ((((uintptr_t)location & MASK16BIT) + bits) <= MASK16BIT)

typedef HashSet<struct DataRace *, uintptr_t, 0, model_malloc, model_calloc, model_free, race_hash, race_equals> RaceSet;

#endif	/* __DATARACE_H__ */
