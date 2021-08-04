#ifndef ACTIONLIST_H
#define ACTIONLIST_H

#include "classlist.h"
#include "stl-model.h"

#define ISACT ((uintptr_t) 1ULL)
#define ACTMASK (~ISACT)

#define ALLBITS 4
#define ALLNODESIZE (1 << ALLBITS)
#define ALLMASK ((1 << ALLBITS)-1)
#define MODELCLOCKBITS 32

class allnode;
void decrementCount(allnode *);

class allnode {
public:
	allnode();
	~allnode();
	MEMALLOC;

private:
	allnode * parent;
	allnode * children[ALLNODESIZE];
	int count;
	mllnode<ModelAction *> * findPrev(modelclock_t index);
	friend class actionlist;
	friend void decrementCount(allnode *);
};

class actionlist {
public:
	actionlist();
	~actionlist();
	void addAction(ModelAction * act);
	mllnode<ModelAction *> * getAction(modelclock_t clock);
	void removeAction(ModelAction * act);
	void clear();
	void clearAndDeleteActions();
	bool isEmpty();
	uint size() {return _size;}
	mllnode<ModelAction *> * begin() {return head;}
	mllnode<ModelAction *> * end() {return tail;}

	MEMALLOC;
private:
	allnode root;
	mllnode<ModelAction *> * head;
	mllnode<ModelAction* > * tail;

	uint _size;
};
#endif
