#include "plugins.h"
#include "pmverifier.h"
#include "persistrace.h"

PMVerifier * pmverifier = NULL;
ModelVector<Analysis *>* registeredAnalyses = NULL;
ModelVector<Analysis *>* installedAnalyses = NULL;

void registerAnalyses() {
	registeredAnalyses = new ModelVector<Analysis *>();
	installedAnalyses = new ModelVector<Analysis *>();
	registeredAnalyses->push_back( new PMVerifier());
	registeredAnalyses->push_back( new PersistRace());
}

void enableAnalysis(const char *name) {
	for(uint i=0;i<registeredAnalyses->size();i++) {
		Analysis *a = (*registeredAnalyses)[i];
		if(strcmp(a->getName(), name ) == 0) {
			installedAnalyses->push_back(a);
		}
	}
}

ModelVector<Analysis*> *getInstalledAnalyses() {
	return installedAnalyses;
}
