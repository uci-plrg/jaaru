#ifndef PLUGIN_H
#define PLUGIN_H
#include "classlist.h"

void registerAnalyses();
void enableAnalysis(const char *name);
ModelVector<Analysis*> *getInstalledAnalyses();

#endif