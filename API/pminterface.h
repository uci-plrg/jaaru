#ifndef PMINTERFACE_H
#define PMINTERFACE_H

extern "C" {
void * getRegionFromID(uint ID);
void setRegionFromID(uint ID, void *ptr);
}
#endif
