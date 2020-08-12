#include <stdio.h>
#include <pthread.h>
#include <atomic>
#include <cstdlib>
#include "atomicapi.h"
#include "pmcheckapi.h"
#include "test.h"
#include "mylibpmem.h"
#include "map.h"
#include "mymemory.h"
#include "pminterface.h"
#include "common.h"

using namespace std;
#define NUMTHREADS 2

atomic<unsigned int> x(0);
atomic<unsigned int> y(0);

extern "C" {
__attribute__ ((visibility ("default"))) void restart();
}

void restart(){
	model_pmem_map_file_init();
	char* addr1 = (char*)model_pmem_map_file("/path/file", 64);
	/*get correct addr*/
	char* addr2 = (char*)model_pmem_file("/path/file");
    (*addr1) = 'a';
    *(addr1+1) = 'b';
    *(addr1+2) = 'c';
    *(addr1+3) = 'd';
	fprintf(stdout,"-----%u-----\n\n", addr1);
	fprintf(stdout,"-----%u-----\n\n", addr2);
	/*is pmem correct*/
	int ispmem = model_pmem_is_pmem(addr1,64);
	fprintf(stdout,"-----%u-----\n\n", ispmem);
	/*persist 1*/
    (*addr1) = 'e';
    *(addr1+1) = 'f';
    *(addr1+2) = 'g';
    *(addr1+3) = 'h';
	model_pmem_persist(addr1,64);
	fprintf(stdout,"-----%s-----\n\n", addr1);
	/*persist 2*/
    (*addr1) = 'i';
    *(addr1+1) = 'j';
    *(addr1+2) = 'k';
    *(addr1+3) = 'l';
	model_pmem_flush(addr1,256);
	model_pmem_drain();
	fprintf(stdout,"-----%s-----\n\n", *((char**)addr1));
	/*move*/
	model_pmem_memmove(addr1+2, addr1, 1);
	/*copy*/
	/*set*/

}

int main() {
	try{
		restart();
	}catch(...) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

