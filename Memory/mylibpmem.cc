#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <mymemory.h>
#include <atomicapi.h>
#include <map.h>
#include <stdio.h>
#include <string.h>
#include <snapshot.h>
#include "pminterface.h"
#include "mylibpmem.h"
#include "mymemory.h"

void
model_pmem_drain(void)
{
	pmc_mfence(); //as assigned by init.c, assuming clwb's existance
}

void
model_pmem_flush(const void *addr, size_t len)
{
	char * tmpaddr = (char *) addr;
	for(char* i=0; i<(char*)len; i=i+1){
		pmc_clflushopt(tmpaddr++); //as assigned by init.c, assuming clwb's existance
	}
}
void
model_pmem_flush_deep(const void *addr, size_t len)
{
	model_pmem_flush(addr,len);
}
//The semantics of pmem_deep_flush() function is the same as pmem_flush() function except that pmem_deep_flush() is indifferent to PMEM_NO_FLUSH environment variable 

void
model_pmem_persist(const void *addr, size_t len)
{
	model_pmem_flush(addr, len);
	puts("--------------10---------------\n\n");
	model_pmem_drain();
}

void
model_pmem_msync(const void *addr, size_t len)
{
	model_pmem_persist(addr,len); 
}
//since we're not calling msync, we don't need to round to pagesize

int
model_pmem_is_pmem(const void *addr, size_t len){
	if (pmemBase==NULL){
		pmemBase = getRegionFromID(0);
	}
	int out = addr<=pmemBase && addr+len<=pmemBase+SHARED_MEMORY_DEFAULT;
	return out;
}

void 
model_pmem_map_file_init(){
	if (getRegionFromID(0) == NULL) {
		//if(sStaticSpace==NULL){
		//	sStaticSpace = create_shared_mspace();
		//}
		setRegionFromID(0, (void*)sStaticSpace);
		//if fork snap is null
	}
	pathToAddr = {{0},NULL,NULL,foo_strhash};
}

void *
model_pmem_map_file(const char *path, size_t len)
{
	void * addr = model_malloc(len);
	insert(&pathToAddr,path,addr);
	return addr;
}

//not in pmem.c: just for our file mapping
void*
model_pmem_file(const char *path){
	return get(&pathToAddr, path);
}

void
pmem_unmap(void *addr, size_t len)
{
	model_free(addr);
}



//ask about:

//pmem_is_pmem, specifically, if we can tell if a memory range is pmem or not
//pmem_file, specifically if we have mapping pmem
//cpy, move,

//ask about implementation
void * model_pmem_memmove(void *pmemdest, const void *src, size_t len){
	if(src>pmemdest){
		for(int i=0; i<len; i++){
			*(((char*)pmemdest)+i) = *(((char*)src)+i);
		}
	}else{
		for(size_t i=len; i>=1; i--){ //size_t can't be 0?
			*(((char*)pmemdest)+i) = *(((char*)src)+i);
		}
	}
	model_pmem_flush(pmemdest,len);
	//just moves and flushes to persistance
}
void * model_pmem_memcpy(void *pmemdest, const void *src, size_t len){
	for(int i=0; i<len; i++){
		*(((char*)pmemdest)+i) = *(((char*)src)+i);
	}
	model_pmem_flush(pmemdest,len);
	//does not handel overlapping memory
}
void * model_pmem_memset(void *pmemdest, int c, size_t len){
	for(int i=0; i<len; i++){
		*(((char*)pmemdest)+i) = c;
	}
	model_pmem_flush(pmemdest,len);
}

void * model_pmem_memmove_nodrain(void *pmemdest, const void *src, size_t len){
	if(src>pmemdest){
		for(int i=0; i<len; i++){
			*(((char*)pmemdest)+i) = *(((char*)src)+i);
		}
	}else{
		for(size_t i=len-1; i>=0; i--){
			*(((char*)pmemdest)+i) = *(((char*)src)+i);
		}
	}
	//just moves
}
void * model_pmem_memcpy_nodrain(void *pmemdest, const void *src, size_t len){
	for(int i=0; i<len; i++){
		*(((char*)pmemdest)+i) = *(((char*)src)+i);
	}
	//does not handel overlapping memory
}
void * model_pmem_memset_nodrain(void *pmemdest, int c, size_t len){
	for(int i=0; i<len; i++){
		*(((char*)pmemdest)+i) = c;
	}
}




