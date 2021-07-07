#include "range.h"
#include "common.h"

bool Range::mergeBeginRange(modelclock_t begin) {
	if(begin > beginR) {
		beginR = begin;
		return true;
	}
	return false;
}
bool Range::minMergeEndgeRange(modelclock_t end) {
	if(end < endR) {
		endR = end;
		return true;
	}
	return false;
}

bool Range::hastIntersection(Range &r) {
	return !(r.beginR > endR || r.endR < beginR);
}


void Range::print() const {
	model_print("%u => %u\n", beginR, endR);
}