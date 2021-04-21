#ifndef RANGE_H
#define RANGE_H
/**
 * Range class is a general class used in cacheline and PMVerifer. Please look at
 * pmverfier.h and cacheline.h to see what beginR and endR are used for.
 */
class Range {
public:
	Range() : beginR(0), endR(0) {}
	modelclock_t getBeginRange(){ return beginR; }
	modelclock_t getEndRange(){ return endR; }
	void setBeginRange(modelclock_t begin) { beginR = begin; }
	void setEndRange(modelclock_t end) { endR = end; }

	MEMALLOC;
protected:
	modelclock_t beginR;
	modelclock_t endR;
};

#endif