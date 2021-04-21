#include "pmverifier.h"

/**
 * This simple analysis updates the endRange for all threads when an execution crashes.
 */
void PMVerifier::crashAnalysis(ModelExecution * execution) {

}

/**
 * This analysis checks all writes that the execution may possibly read from are in range.
 */
void PMVerifier::mayReadFromAnalysis(ModelAction *read, SnapVector<SnapVector<Pair<ModelExecution *, ModelAction *> > *> *rf_set) {
}

/**
 * This analysis checks for the conflict and if it found none, updates the beginRange for all threads.
 * If the write is RMW, it finds the first next write in each thread and update their endRanges. Otherwise,
 * if the write was a normal write, it updates the threads' endRanges so they don't read from out of range
 * writes.
 */
void PMVerifier::readFromWriteAnalysis(ModelAction *curr, SnapVector<Pair<ModelExecution *, ModelAction *> > *rfarray) {
}