#ifndef PLUGIN_H
#define PLUGIN_H
#include "classlist.h"
#include "analysis.h"

void registerAnalyses();
void enableAnalysis(const char *name);
ModelVector<Analysis*> *getInstalledAnalyses();

#endif