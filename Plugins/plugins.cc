#include "plugins.h"
#include "pmverifier.h"

PMVerifier * pmverifier = NULL;

inline void registerVerifier() {
    pmverifier = new PMVerifier();
}

inline PMVerifier *getPMVerifier() {
    return pmverifier;
}
